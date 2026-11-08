#include "EncoderTask.hpp"

static const char* const channel_names[AMP_CHANNEL_COUNT] = {"Clean", "Drive", "More Drive"};

// Menu entries: MIDI_OMNI_SETTING (ALL), the channels 1-16, then ESC.
#define MENU_ESC     17
#define MENU_ENTRIES 18


EncoderTask::EncoderTask(RotaryEncoder& encoder, Button& store,
                         PresetLoader& presets, OpticalRelay& relay,
                         SegmentDisplay& display, MidiSettings& midi,
                         Led& led_green,
                         Led& led_yellow, Led& led_red)
    : encoder_{encoder}
    , store_{store}
    , presets_{presets}
    , relay_{relay}
    , display_{display}
    , midi_{midi}
    , led_green_{led_green}
    , led_yellow_{led_yellow}
    , led_red_{led_red}
{
}

void EncoderTask::run()
{
    // Read every input on every pass, whatever the state, so no press is
    // left over for a later state.
    const Input in{encoder_.moved(), encoder_.enter_pressed(), store_.pressed()};
    switch (state_)
    {
    case State::live:
        runLive(in);
        break;
    case State::browsing:
        runBrowsing(in);
        break;
    case State::menu:
        runMenu(in);
        break;
    }
}

// In every state a button press wins over steps in the same pass: a press is
// rare and deliberate, while a step taken together with it is mostly a jolt
// of the knob.

void EncoderTask::runLive(const Input& in)
{
    if (in.enter)
    {
        openMenu();
    }
    else if (in.store)
    {
        store();
    }
    else if (in.step != 0)
    {
        browse(in.step);
    }
}

void EncoderTask::runBrowsing(const Input& in)
{
    if (in.enter || in.store)
    {
        // A button doesn't wait for the encoder to rest: the amp catches up
        // first, then the press counts as in live.
        apply();
        runLive({0, in.enter, in.store});
    }
    else if (in.step != 0)
    {
        browse(in.step);
    }
    else if (millis() - moved_at_ >= BROWSE_SETTLE_MS)
    {
        apply();
    }
}

void EncoderTask::runMenu(const Input& in)
{
    if (in.enter || in.store)
    {
        confirmMenu();
    }
    else if (in.step != 0)
    {
        int entry = (menu_entry_ + in.step) % MENU_ENTRIES;
        menu_entry_ = entry < 0 ? entry + MENU_ENTRIES : entry;
        printMenuEntry();
        show();
    }
}

void EncoderTask::browse(int step)
{
    int number = (presets_.number() + step) % PRESET_COUNT;
    presets_.select(number < 0 ? number + PRESET_COUNT : number);
    state_ = State::browsing;
    moved_at_ = millis();
    // Show the program now, switch the amp once the encoder rests.
    show();
}

void EncoderTask::select(uint8_t number)
{
    presets_.select(number);
    apply();
}

void EncoderTask::apply()
{
    if (state_ == State::browsing)
    {
        state_ = State::live;
    }
    const Preset& working = presets_.working();
    relay_.select(working.channel());
    showChannel();
    Serial.print(F("program="));
    Serial.print(presets_.number() + 1);
    Serial.print(F(" channel="));
    Serial.print(channel_names[channel_index(working.channel())]);
    Serial.println(presets_.dirty() ? F(" (modified)") : F(""));
    show();
}

void EncoderTask::store()
{
    if (!presets_.dirty())
    {
        // Nothing to store: a short wink shows the press was registered.
        channelLed(presets_.working().channel()).wink(NOTHING_TO_STORE_MS);
        Serial.println(F("store: nothing to store"));
        return;
    }
    presets_.store();
    Serial.print(F("stored program="));
    Serial.print(presets_.number() + 1);
    Serial.print(F(" channel="));
    Serial.println(channel_names[channel_index(presets_.working().channel())]);
    // Clean again: the channel LED stops swelling.
    showChannel();
}

void EncoderTask::showChannel()
{
    AmpChannel active = presets_.working().channel();
    bool dirty = presets_.dirty();
    for (uint8_t i = 0; i < AMP_CHANNEL_COUNT; ++i)
    {
        AmpChannel channel = static_cast<AmpChannel>(i);
        Led& led = channelLed(channel);
        if (channel != active)
        {
            led.off();
        }
        else if (dirty)
        {
            led.swell();
        }
        else
        {
            led.on();
        }
    }
}

Led& EncoderTask::channelLed(AmpChannel channel)
{
    switch (channel)
    {
    case AmpChannel::drive:
        return led_yellow_;
    case AmpChannel::more_drive:
        return led_red_;
    case AmpChannel::clean:
    default:
        return led_green_;
    }
}

void EncoderTask::openMenu()
{
    state_ = State::menu;
    menu_entry_ = midi_.setting();
    Serial.println(F("menu on"));
    printMenuEntry();
    show();
}

void EncoderTask::confirmMenu()
{
    if (menu_entry_ == MENU_ESC)
    {
        Serial.println(F("midi: not saved"));
    }
    else
    {
        midi_.store(menu_entry_);
        if (menu_entry_ == MIDI_OMNI_SETTING)
        {
            Serial.println(F("midi: saved omni"));
        }
        else
        {
            Serial.print(F("midi: saved channel="));
            Serial.println(menu_entry_);
        }
    }
    state_ = State::live;
    Serial.println(F("menu off"));
    show();
}

void EncoderTask::printMenuEntry()
{
    Serial.print(F("menu: "));
    if (menu_entry_ == MIDI_OMNI_SETTING)
    {
        Serial.println(F("ALL"));
    }
    else if (menu_entry_ == MENU_ESC)
    {
        Serial.println(F("ESC"));
    }
    else
    {
        Serial.print(F("CH:"));
        Serial.print(menu_entry_ / 10);
        Serial.println(menu_entry_ % 10);
    }
}

void EncoderTask::show()
{
    if (state_ != State::menu)
    {
        display_.showNumber(presets_.number() + 1);
    }
    else if (menu_entry_ == MIDI_OMNI_SETTING)
    {
        display_.showText("ALL");
    }
    else if (menu_entry_ == MENU_ESC)
    {
        display_.showText("ESC");
    }
    else
    {
        // "CH:05": the colon sits between the second and third digit.
        const char text[] = {'C', 'H', char('0' + menu_entry_ / 10),
                             char('0' + menu_entry_ % 10), '\0'};
        display_.showText(text, true);
    }
}
