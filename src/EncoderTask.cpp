#include "EncoderTask.hpp"

static const char* const channel_names[AMP_CHANNEL_COUNT] = {"Clean", "Drive", "More Drive"};


EncoderTask::EncoderTask(RotaryEncoder& encoder, Button& store,
                         PresetLoader& presets, OpticalRelay& relay,
                         LcdBuffer& lcd_buffer, Led& led_green,
                         Led& led_yellow, Led& led_red)
    : encoder_{encoder}
    , store_{store}
    , presets_{presets}
    , relay_{relay}
    , lcd_buffer_{lcd_buffer}
    , led_green_{led_green}
    , led_yellow_{led_yellow}
    , led_red_{led_red}
{
}

void EncoderTask::run()
{
    int step = encoder_.moved();
    if (step != 0 && !menu_)
    {
        uint8_t number = presets_.number();
        presets_.select((number + PRESET_COUNT + step) % PRESET_COUNT);
        // Show the program now, switch the amp once the encoder rests.
        show();
        apply_pending_ = true;
        moved_at_ = millis();
    }
    else if (apply_pending_ && millis() - moved_at_ >= BROWSE_SETTLE_MS)
    {
        apply();
    }
    if (encoder_.enter_pressed())
    {
        menu_ = !menu_;
        Serial.println(menu_ ? F("menu on") : F("menu off"));
        show();
    }
    if (store_.pressed())
    {
        if (menu_)
        {
            // To come: confirm the MIDI settings and write them to EEPROM.
            Serial.println(F("store: no MIDI settings yet"));
        }
        else
        {
            store();
        }
    }
}

void EncoderTask::apply()
{
    apply_pending_ = false;
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
        return;
    }
    presets_.store();
    Serial.print(F("stored program="));
    Serial.print(presets_.number() + 1);
    Serial.print(F(" channel="));
    Serial.println(channel_names[channel_index(presets_.working().channel())]);
    // Clean again: the channel LED stops swelling, the "*" goes away.
    showChannel();
    show();
}

void EncoderTask::showChannel()
{
    AmpChannel channel = presets_.working().channel();
    bool dirty = presets_.dirty();
    Led* leds[] = {&led_green_, &led_yellow_, &led_red_};
    for (uint8_t i = 0; i < AMP_CHANNEL_COUNT; ++i)
    {
        if (i != channel_index(channel))
        {
            leds[i]->off();
        }
        else if (dirty)
        {
            leds[i]->swell();
        }
        else
        {
            leds[i]->on();
        }
    }
}

void EncoderTask::show()
{
    char line[LCD_COLS + 1];
    if (menu_)
    {
        snprintf(line, sizeof(line), "Menu");
    }
    else
    {
        // "P128 More Drive*" is exactly 16 characters.
        snprintf(line, sizeof(line), "P%-3u %s%s", presets_.number() + 1,
                 channel_names[channel_index(presets_.working().channel())],
                 presets_.dirty() ? "*" : "");
    }
    lcd_buffer_.write(1, 0, line, LCD_COLS);
}
