/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdlib.h>
#include <math.h>
#include "hardware.h"
#include "clcd.h"
#include "control_chain.h"
#include "chip.h"
#include "config.h"
#include "util.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

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



/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static char* reverse(char* str, uint32_t str_len)
{
    char *end = str + (str_len - 1);
    char *start = str, tmp;

    while (start < end)
    {
        tmp = *end;
        *end = *start;
        *start = tmp;

        start++;
        end--;
    }

    return str;
}


static void all_leds(int color, int status)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; ++i)
        hw_led(i, color, status);
}



/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision)
{
    int p = 1;
    int i = 0;
    while (i < precision) {
        p *= 10;
        i++;
    }
    uint32_t len = 0;
    uint8_t need_minus = 0;
    long int_part;
    long decimal_part;
    if (num >=0) {
        int_part = (long) (num + 0.5f / p);
    } else {
        int_part = (long) (num - 0.5f / p);
        need_minus = 1;
    }
    len += int_to_str(int_part, string, string_size, 0, need_minus);
    while (*string != '\0') string++;
    *string++ = '.';
    len++;
    if (num >= 0.0f) {
        decimal_part = fabsf((long)((num - int_part) * p + 0.5f));
    } else {
        decimal_part = fabsf((long)((num - int_part) * p - 0.5f));
    }
    if (len < string_size) len += int_to_str(decimal_part, string, string_size - len, precision, 0);
    return len;
}

uint32_t int_to_str(int32_t num, char *string, uint32_t string_size, uint8_t zero_leading, uint8_t need_minus)
{
    char *pstr = string;
    uint8_t signal = 0;
    uint32_t str_len;

    if (!string) return 0;
{
    // exception case: number is zero
    if (num == 0)
    {
        *pstr++ = '0';
        if (zero_leading) zero_leading--;
    }

    // need minus signal?
    if (num < 0 || (num == 0 && need_minus))
    {
        num = -num;
        signal = 1;
        string_size--;
    }

    // composes the string
    while (num)
    {
        *pstr++ = (num % 10) + '0';
        num /= 10;

        if (--string_size == 0) break;
        if (zero_leading) zero_leading--;
    }

    // checks buffer size
    if (string_size == 0)
    {
        *string = 0;
        return 0;
    }

    // fills the zeros leading
    while (zero_leading--) *pstr++ = '0';

    // put the minus if necessary
    if (signal) *pstr++ = '-';
    *pstr = 0;

    // invert the string characters
    str_len = (pstr - string);
    reverse(string, str_len);

    return str_len;
}


    while (1)
    {
        // test buttons
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                //first button means exit (or in the furute next menu item)
                if (i == 0)
                {
                    // ask user to reboot
                    clcd_cursor_set(0, CLCD_LINE1, 0);
                    clcd_print(0, "NEW SETTINGS    ");
                    clcd_cursor_set(0, CLCD_LINE2, 0);
                    clcd_print(0, "SAVED           ");

                    // ... and test the switches
                    clcd_cursor_set(1, CLCD_LINE1, 0);
                    clcd_print(1, "PLEASE REBOOT   ");
                    clcd_cursor_set(1, CLCD_LINE2, 0);
                    clcd_print(1, "THE DEVICE      ");

                    //clear prev colors
                    for (int i = 0; i < 3; ++i)
                        all_leds(i, LED_OFF);

                    all_leds(LED_G, LED_ON);

                    //lock the device till reboot
                    while (1); 
                }
                
                
            }
        }
        delay_ms(5);
    }
}
