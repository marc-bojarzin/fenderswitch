#include "SegmentDisplay.hpp"

// Segment bits SEG_A to SEG_G come from TM1637Display.h:
//
//      -A-
//     F   B
//      -G-
//     E   C
//      -D-

// On the 4-digit clock modules the colon hangs on the "decimal point" bit of
// the second digit.
#define COLON_DIGIT 1
#define COLON_BIT   0x80

// Digits 0-9.
static const uint8_t digit_font[10] PROGMEM = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B | SEG_C,                                         // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                 // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                 // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                         // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,         // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,         // 9
};

// Letters A-Z. Some only exist in lower case (b, d, n, r, t, ...), some not
// at all and borrow a look-alike (K = H, M = N, V/W = U, X = H).
static const uint8_t letter_font[26] PROGMEM = {
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,         // A
    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,                 // b
    SEG_A | SEG_D | SEG_E | SEG_F,                         // C
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,                 // d
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,                 // E
    SEG_A | SEG_E | SEG_F | SEG_G,                         // F
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,                 // G
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,                 // H
    SEG_E | SEG_F,                                         // I
    SEG_B | SEG_C | SEG_D | SEG_E,                         // J
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,                 // K (as H)
    SEG_D | SEG_E | SEG_F,                                 // L
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F,                 // M (as N)
    SEG_C | SEG_E | SEG_G,                                 // n
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // O
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,                 // P
    SEG_A | SEG_B | SEG_C | SEG_F | SEG_G,                 // q
    SEG_E | SEG_G,                                         // r
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 // S
    SEG_D | SEG_E | SEG_F | SEG_G,                         // t
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,                 // U
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,                 // V (as U)
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,                 // W (as U)
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,                 // X (as H)
    SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,                 // y
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                 // Z
};

SegmentDisplay::SegmentDisplay(uint8_t clk_pin, uint8_t dio_pin)
    : tm_{clk_pin, dio_pin, SEGMENT_BIT_DELAY_US}
{
    memset(digits_, 0, sizeof(digits_));
}

void SegmentDisplay::begin(uint8_t brightness)
{
    valid_ = false;
    setBrightness(brightness);
    clear();
}

void SegmentDisplay::setBrightness(uint8_t brightness)
{
    brightness_ = brightness > 7 ? 7 : brightness;
    tm_.setBrightness(brightness_);
    // The chip only takes the brightness with the next write.
    valid_ = false;
    update(digits_, colon_);
}

void SegmentDisplay::clear()
{
    const uint8_t blank[SEGMENT_DIGITS] = {};
    update(blank, false);
}

void SegmentDisplay::showNumber(uint16_t value, bool colon)
{
    uint8_t digits[SEGMENT_DIGITS];
    if (value > 9999)
    {
        memset(digits, SEG_G, sizeof(digits));
    }
    else
    {
        for (int8_t i = SEGMENT_DIGITS - 1; i >= 0; --i)
        {
            // The last digit always shows, even for 0.
            bool blank = value == 0 && i < SEGMENT_DIGITS - 1;
            digits[i] = blank ? 0 : encode('0' + value % 10);
            value /= 10;
        }
    }
    update(digits, colon);
}

void SegmentDisplay::showText(const char* text, bool colon)
{
    uint8_t digits[SEGMENT_DIGITS];
    for (uint8_t i = 0; i < SEGMENT_DIGITS; ++i)
    {
        digits[i] = *text ? encode(*text++) : 0;
    }
    update(digits, colon);
}

void SegmentDisplay::showSegments(const uint8_t segments[SEGMENT_DIGITS])
{
    update(segments, colon_);
}

void SegmentDisplay::setColon(bool on)
{
    update(digits_, on);
}

uint8_t SegmentDisplay::encode(char c)
{
    if (c >= '0' && c <= '9')
    {
        return pgm_read_byte(&digit_font[c - '0']);
    }
    // Lower-case shapes that differ from the upper-case ones.
    switch (c)
    {
    case 'c': return SEG_D | SEG_E | SEG_G;
    case 'h': return SEG_C | SEG_E | SEG_F | SEG_G;
    case 'i': return SEG_E;
    case 'o': return SEG_C | SEG_D | SEG_E | SEG_G;
    case 'u': return SEG_C | SEG_D | SEG_E;
    case '-': return SEG_G;
    case '_': return SEG_D;
    default: break;
    }
    if (c >= 'a' && c <= 'z')
    {
        c -= 'a' - 'A';
    }
    if (c >= 'A' && c <= 'Z')
    {
        return pgm_read_byte(&letter_font[c - 'A']);
    }
    return 0;
}

void SegmentDisplay::update(const uint8_t digits[SEGMENT_DIGITS], bool colon)
{
    if (valid_ && colon == colon_ &&
        memcmp(digits, digits_, SEGMENT_DIGITS) == 0)
    {
        return;
    }
    if (digits != digits_)
    {
        memcpy(digits_, digits, SEGMENT_DIGITS);
    }
    colon_ = colon;
    valid_ = true;

    uint8_t out[SEGMENT_DIGITS];
    memcpy(out, digits_, SEGMENT_DIGITS);
    if (colon_)
    {
        out[COLON_DIGIT] |= COLON_BIT;
    }
    tm_.setSegments(out);
}
