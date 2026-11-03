#include "LcdBuffer.hpp"

LcdBuffer::LcdBuffer(LiquidCrystal_I2C* lcd)
    : lcd_{lcd}
{
    clear();
}

void LcdBuffer::clear()
{
    memset(lines_, ' ', sizeof(lines_));
}

void LcdBuffer::update(uint8_t row, const char* content)
{
    for (uint8_t i = 0; i < LCD_COLS; ++i)
    {
        put(row, i, content[i]);
    }
}

void LcdBuffer::write(uint8_t row, uint8_t col, const char* text,
                      uint8_t width)
{
    for (uint8_t i = 0; i < width && col + i < LCD_COLS; ++i)
    {
        char c = *text ? *text++ : ' ';
        put(row, col + i, c);
    }
}

void LcdBuffer::put(uint8_t row, uint8_t col, char c)
{
    if (row >= LCD_ROWS || col >= LCD_COLS || lines_[row][col] == c)
    {
        return;
    }
    lines_[row][col] = c;
    lcd_->setCursor(col, row);
    lcd_->write(c);
}
