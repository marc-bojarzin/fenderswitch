#ifndef LCD_BUFFER_HPP
#define LCD_BUFFER_HPP

#include <LiquidCrystal_I2C.h>

// Number of rows and columns of the LCD.
#define LCD_ROWS 2
#define LCD_COLS 16

// Shadow buffer in front of a 16x2 character LCD.
//
// Keeps a copy of what each row shows and writes only the characters that
// changed, which avoids flicker and keeps the slow I2C traffic low. All text
// should go through this class; anything written to the LCD directly isn't in
// the copy.
class LcdBuffer
{
    LiquidCrystal_I2C* lcd_;

    // What each row currently shows.
    char lines_[LCD_ROWS][LCD_COLS];

public:
    // Not default constructible.
    LcdBuffer() = delete;

    // Constructor. Assumes a blank LCD.
    LcdBuffer(LiquidCrystal_I2C* lcd);

    // Resets the copy to blank, without touching the LCD. Call it after
    // clearing the LCD directly.
    void clear();

    // Shows content (exactly LCD_COLS characters, no terminator needed) on
    // row. Rows outside 0 to LCD_ROWS - 1 are ignored.
    void update(uint8_t row, const char* content);

    // Shows text on row from col on, padded with spaces to width columns.
    // Stops at the end of the row.
    void write(uint8_t row, uint8_t col, const char* text, uint8_t width);

private:
    // Shows c at row, col if it differs from the copy.
    void put(uint8_t row, uint8_t col, char c);
};

#endif // LCD_BUFFER_HPP
