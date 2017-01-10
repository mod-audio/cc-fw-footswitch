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

#ifndef CLCD_H
#define CLCD_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "delay.h"
#include "gpio.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

// library version
#define CLCD_VERSION        "0.0.0"

// flags for function set
#define CLCD_8BIT   0x10
#define CLCD_4BIT   0x00
#define CLCD_2LINE  0x08
#define CLCD_1LINE  0x00
#define CLCD_5x10   0x04
#define CLCD_5x8    0x00

// flags for display control
#define CLCD_ON         0x04
#define CLCD_OFF        0x00
#define CLCD_CURSOR_ON  0x02
#define CLCD_CURSOR_OFF 0x00
#define CLCD_BLINK_ON   0x01
#define CLCD_BLINK_OFF  0x00

// flags for entry mode
#define CLCD_SHIFT_RIGHT    0x00
#define CLCD_SHIFT_LEFT     0x02
#define CLCD_INCREMENT      0x01
#define CLCD_DECREMENT      0x00


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

#define CLCD_MAX_DISPLAYS   2


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/


typedef struct clcd_gpio_t {
    gpio_t rs, rw, en;
    gpio_t data[8];
} clcd_gpio_t;

enum {CLCD_LINE1, CLCD_LINE2};


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

int clcd_init(uint8_t config, const clcd_gpio_t *gpio);
void clcd_control(int lcd_id, int on_off);
void clcd_clear(int lcd_id);
void clcd_print(int lcd_id, const char *str);
void clcd_cursor_set(int lcd_id, int line, int col);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/


#ifdef __cplusplus
}
#endif

#endif
