#ifndef SEGMENT_DISPLAY_HPP
#define SEGMENT_DISPLAY_HPP

#include <Arduino.h>
#include <TM1637Display.h>

// Number of digits on the display.
#define SEGMENT_DIGITS 4

// Half clock period of the TM1637 bus, in us. The library default is 100,
// which makes a full update take about 20 ms; 50 halves that. Raise it if the
// display shows garbage (long wires, filter capacitors on the module).
#ifndef SEGMENT_BIT_DELAY_US
#define SEGMENT_BIT_DELAY_US 50
#endif

// Brightness at startup, 0 (dimmest) to 7 (brightest).
#ifndef SEGMENT_BRIGHTNESS
#define SEGMENT_BRIGHTNESS 4
#endif

// 4-digit 7-segment display with a colon, driven by a TM1637 (CLK/DIO).
//
// The TM1637 speaks its own two-wire protocol, not I2C, so it needs two pins
// of its own and can't share a bus with I2C devices.
//
// Keeps a copy of what the display shows and only talks to the chip when
// something changed: the library bit-bangs the bus with delays, so every
// update blocks for a few ms.
class SegmentDisplay
{
    TM1637Display tm_;

    // Segments of each digit, left to right, colon not included.
    uint8_t digits_[SEGMENT_DIGITS];
    bool colon_ = false;
    // Brightness 0-7.
    uint8_t brightness_ = SEGMENT_BRIGHTNESS;
    // False until the first write, so begin() always reaches the chip.
    bool valid_ = false;

public:
    // Not default constructible.
    SegmentDisplay() = delete;

    // Constructor. Doesn't touch the pins yet; call begin() in setup().
    SegmentDisplay(uint8_t clk_pin, uint8_t dio_pin);

    // Sets the brightness and blanks the display.
    void begin(uint8_t brightness = SEGMENT_BRIGHTNESS);

    // Sets the brightness, 0 (dimmest) to 7 (brightest).
    void setBrightness(uint8_t brightness);

    // Blanks all digits and the colon.
    void clear();

    // Shows value right-aligned without leading zeros, e.g. "  12". Values
    // above 9999 show "----". Keeps the colon.
    void showNumber(uint16_t value);

    // Shows up to SEGMENT_DIGITS characters left-aligned, padded with blanks,
    // e.g. "Ch16" or "Omni". Knows digits, letters (as far as 7 segments
    // allow), '-', '_' and ' '; anything else stays blank. Keeps the colon.
    void showText(const char* text);

    // Shows raw segment patterns (bit 0 = segment A ... bit 6 = segment G),
    // left to right. Keeps the colon.
    void showSegments(const uint8_t segments[SEGMENT_DIGITS]);

    // Turns the colon between the second and third digit on or off.
    void setColon(bool on);

    // Returns the segment pattern of c, 0 for characters it can't show.
    static uint8_t encode(char c);

private:
    // Sends the copy to the chip if it differs from what was sent last.
    void update(const uint8_t digits[SEGMENT_DIGITS], bool colon);
};

#endif // SEGMENT_DISPLAY_HPP
