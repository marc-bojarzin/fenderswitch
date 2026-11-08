#include "EncoderTask.hpp"

static const char* const channel_names[AMP_CHANNEL_COUNT] = {"Clean", "Drive", "More Drive"};

// Top menu entries.
#define TOP_MIDI     0
#define TOP_DISPLAY  1
#define TOP_ENTRIES  3  // ESC included

// MIDI submenu: MIDI_OMNI_SETTING (ALL), the channels 1-16, then ESC.
#define MIDI_ENTRIES 18

// Display submenu: brightness 0-7 (shown as br:01-br:08), then ESC.
#define DISPLAY_ENTRIES 9

EncoderTask::EncoderTask(RotaryEncoder& encoder, Button& store,
                         PresetLoader& presets, OpticalRelay& relay,
                         SegmentDisplay& display, Settings& settings,
                         Led& led_green,
                         Led& led_yellow, Led& led_red)
    : encoder_{encoder}
    , store_{store}
    , presets_{presets}
    , relay_{relay}
    , display_{display}
    , settings_{settings}
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
    case State::menu_midi:
    case State::menu_display:
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
        int entry = (menu_entry_ + in.step) % menuSize();
        menu_entry_ = entry < 0 ? entry + menuSize() : entry;
        if (state_ == State::menu_display)
        {
            // Preview: each level shows right away, ESC shows the stored one.
            setBrightness(atEsc() ? settings_.brightness() : menu_entry_);
        }
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
    Serial.println(F("menu on"));
    enterMenu(State::menu, TOP_MIDI);
}

void EncoderTask::enterMenu(State state, uint8_t entry)
{
    state_ = state;
    menu_entry_ = entry;
    printMenuEntry();
    show();
}

void EncoderTask::confirmMenu()
{
    switch (state_)
    {
    case State::menu:
        if (menu_entry_ == TOP_MIDI)
        {
            enterMenu(State::menu_midi, settings_.midi());
        }
        else if (menu_entry_ == TOP_DISPLAY)
        {
            enterMenu(State::menu_display, settings_.brightness());
        }
        else
        {
            closeMenu();
        }
        break;
    case State::menu_midi:
        if (atEsc())
        {
            enterMenu(State::menu, TOP_MIDI);
            break;
        }
        settings_.store_midi(menu_entry_);
        if (menu_entry_ == MIDI_OMNI_SETTING)
        {
            Serial.println(F("midi: saved omni"));
        }
        else
        {
            Serial.print(F("midi: saved channel="));
            Serial.println(menu_entry_);
        }
        closeMenu();
        break;
    case State::menu_display:
        if (atEsc())
        {
            // The ESC entry already shows the stored brightness again.
            enterMenu(State::menu, TOP_DISPLAY);
            break;
        }
        settings_.store_brightness(menu_entry_);
        Serial.print(F("display: saved brightness="));
        Serial.println(menu_entry_ + 1);
        closeMenu();
        break;
    default:
        break;
    }
}

void EncoderTask::closeMenu()
{
    state_ = State::live;
    Serial.println(F("menu off"));
    show();
}

uint8_t EncoderTask::menuSize() const
{
    switch (state_)
    {
    case State::menu_midi:
        return MIDI_ENTRIES;
    case State::menu_display:
        return DISPLAY_ENTRIES;
    default:
        return TOP_ENTRIES;
    }
}

bool EncoderTask::atEsc() const
{
    return menu_entry_ == menuSize() - 1;
}

bool EncoderTask::menuText(char text[SEGMENT_DIGITS + 1]) const
{
    // Two-letter prefix plus two digits, e.g. "CH" "05".
    const char* prefix = nullptr;
    uint8_t value = 0;
    if (atEsc())
    {
        strcpy_P(text, PSTR("ESC"));
    }
    else if (state_ == State::menu)
    {
        strcpy_P(text, menu_entry_ == TOP_MIDI ? PSTR("Chan") : PSTR("dISP"));
    }
    else if (state_ == State::menu_midi && menu_entry_ == MIDI_OMNI_SETTING)
    {
        strcpy_P(text, PSTR("ALL"));
    }
    else if (state_ == State::menu_midi)
    {
        prefix = "CH";
        value = menu_entry_;
    }
    else
    {
        prefix = "br";
        value = menu_entry_ + 1;
    }
    if (prefix == nullptr)
    {
        return false;
    }
    text[0] = prefix[0];
    text[1] = prefix[1];
    text[2] = '0' + value / 10;
    text[3] = '0' + value % 10;
    text[4] = '\0';
    return true;
}

void EncoderTask::printMenuEntry()
{
    char text[SEGMENT_DIGITS + 1];
    bool colon = menuText(text);
    Serial.print(F("menu: "));
    if (colon)
    {
        Serial.write(text, 2);
        Serial.print(':');
        Serial.println(text + 2);
    }
    else
    {
        Serial.println(text);
    }
}

void EncoderTask::setBrightness(uint8_t brightness)
{
    display_.setBrightness(brightness);
    Serial.print(F("display: brightness="));
    Serial.println(brightness + 1);
}

void EncoderTask::show()
{
    if (state_ == State::live || state_ == State::browsing)
    {
        display_.showNumber(presets_.number() + 1);
        return;
    }
    char text[SEGMENT_DIGITS + 1];
    bool colon = menuText(text);
    display_.showText(text, colon);
}
