/*
 * clcd - Character LCD Library
 * https://github.com/ricardocrudo/cldc
 *
 * Copyright (c) 2016 Ricardo Crudo <ricardo.crudo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "clcd.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

// commands
#define LCD_CLEAR_DISPLAY   0x01
#define LCD_RETURN_HOME     0x02
#define LCD_ENTRY_MODE_SET  0x04
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_CURSOR_SHIFT    0x10
#define LCD_FUNCTION_SET    0x20
#define LCD_SET_CGRAM_ADDR  0x40
#define LCD_SET_DDRAM_ADDR  0x80


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

typedef struct clcd_t {
    unsigned int interface, control;
    const clcd_gpio_t *gpio;
} clcd_t;

enum {LCD_CMD, LCD_DATA};
enum {LCD_WRITE, LCD_READ};


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

clcd_t g_lcds[CLCD_MAX_DISPLAYS];


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

// enable pulse
static void lcd_enable(clcd_t *lcd)
{
    int port = lcd->gpio->en.port;
    int pin = lcd->gpio->en.pin;

    gpio_set(port, pin, 1);
    delay_us(1);
    gpio_set(port, pin, 0);
}

// check busy flag
void lcd_busy(clcd_t *lcd)
{
    // RW pin is not being used
    if (lcd->gpio->rw.pin < 0)
        return;

    gpio_set(lcd->gpio->rs.port, lcd->gpio->rs.pin, LCD_CMD);
    gpio_set(lcd->gpio->rw.port, lcd->gpio->rw.pin, LCD_READ);

    int d7 = lcd->interface - 1;
    const gpio_t *data = lcd->gpio->data;
    gpio_dir(data[d7].port, data[d7].pin, GPIO_INPUT);

    int busy = 1;
    while (busy)
    {
        gpio_set(lcd->gpio->en.port, lcd->gpio->en.pin, 1);
        delay_us(1);
        busy = gpio_get(data[d7].port, data[d7].pin);
        gpio_set(lcd->gpio->en.port, lcd->gpio->en.pin, 0);

        if (lcd->interface == 4)
            lcd_enable(lcd);
    }

    gpio_dir(data[d7].port, data[d7].pin, GPIO_OUTPUT);
    gpio_set(lcd->gpio->rw.port, lcd->gpio->rw.pin, LCD_WRITE);
}

// write to lcd GPIOs
static void lcd_write(clcd_t *lcd, uint8_t value)
{
    const gpio_t *data = lcd->gpio->data;

    for (int i = 0; i < (int) lcd->interface; i++)
        gpio_set(data[i].port, data[i].pin, (value >> i) & 1);

    lcd_enable(lcd);

    // only uses delay if RW pin is not being used
    //if (lcd->gpio->rw.pin < 0) //FIXME: uncomment me when hw is fixed
        delay_us(50);
}

// send command or data to lcd
static void lcd_send(clcd_t *lcd, uint8_t value, uint8_t cmd_data)
{
    //lcd_busy(lcd); //FIXME: uncomment me when hw is fixed

    gpio_set(lcd->gpio->rs.port, lcd->gpio->rs.pin, cmd_data);

    if (lcd->interface == 4)
        lcd_write(lcd, value >> 4);

    lcd_write(lcd, value);
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

int clcd_init(uint8_t config, const clcd_gpio_t *gpio)
{
    static unsigned int clcd_counter;

    if (clcd_counter >= CLCD_MAX_DISPLAYS)
        return 0;

    int lcd_id = clcd_counter++;
    clcd_t *lcd = &g_lcds[lcd_id];

    // initialize variables
    lcd->gpio = gpio;
    lcd->interface = (config & CLCD_8BIT ? 8 : 4);
    lcd->control = LCD_DISPLAY_CONTROL;

    // configure GPIOs as output
    gpio_dir(gpio->rs.port, gpio->rs.pin, GPIO_OUTPUT);
    gpio_dir(gpio->en.port, gpio->en.pin, GPIO_OUTPUT);

    if (gpio->rw.pin >= 0)
        gpio_dir(gpio->rw.port, gpio->rw.pin, GPIO_OUTPUT);

    for (unsigned int i = 0; i < lcd->interface; i++)
        gpio_dir(gpio->data[i].port, gpio->data[i].pin, GPIO_OUTPUT);

    // see datasheet pages 45, 46 for initialization proceeding
    // https://www.sparkfun.com/datasheets/LCD/HD44780.pdf

    // display initialization time
    delay_us(50000);

    gpio_set(gpio->rs.port, gpio->rs.pin, 0);
    gpio_set(gpio->en.port, gpio->en.pin, 0);

    if (gpio->rw.pin >= 0)
        gpio_set(gpio->rw.port, gpio->rw.pin, LCD_WRITE);

    // initialization in 4 bits interface
    if (lcd->interface == 4)
    {
        // function set
        lcd_write(lcd, 0x03);
        delay_us(4500);

        // function set
        lcd_write(lcd, 0x03);
        delay_us(150);

        // function set
        lcd_write(lcd, 0x03);
        delay_us(150);

        lcd_write(lcd, 0x02);
    }
    else
    {
        lcd_send(lcd, config, LCD_CMD);
        delay_us(4500);

        lcd_send(lcd, config, LCD_CMD);
        delay_us(150);

        lcd_send(lcd, config, LCD_CMD);
    }

    // set interface, number of lines and font size
    config |= LCD_FUNCTION_SET;
    lcd_send(lcd, config, LCD_CMD);

    // turn display on (cursor and blinking is off by default)
    clcd_control(lcd_id, CLCD_ON);

    // clear display
    clcd_clear(lcd_id);

    // entry mode
    uint8_t entry = LCD_ENTRY_MODE_SET | CLCD_SHIFT_LEFT | CLCD_DECREMENT;
    lcd_send(lcd, entry, LCD_CMD);

    return lcd_id;
}

void clcd_control(int lcd_id, int on_off)
{
    clcd_t *lcd = &g_lcds[lcd_id];
    lcd->control &= ~CLCD_ON;
    lcd->control |= on_off;

    lcd_send(lcd, lcd->control, LCD_CMD);
}

void clcd_clear(int lcd_id)
{
    clcd_t *lcd = &g_lcds[lcd_id];
    lcd_send(lcd, LCD_CLEAR_DISPLAY, LCD_CMD);
    delay_us(2000);
}

void clcd_print(int lcd_id, const char *str)
{
    clcd_t *lcd = &g_lcds[lcd_id];
    const char *pstr = str;

    while (*pstr)
        lcd_send(lcd, *pstr++, LCD_DATA);
}

void clcd_cursor_set(int lcd_id, int line, int col)
{
    clcd_t *lcd = &g_lcds[lcd_id];

    line = (line << 6) & 0x40;
    uint8_t address = LCD_SET_DDRAM_ADDR | (line + col);
    lcd_send(lcd, address, LCD_CMD);
}
