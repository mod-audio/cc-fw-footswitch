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
#include "settings.h"


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
#define     MEM_SIZE                    256     //we can only write in powers of 256

//EEPROMP memory define's
#define     TARGET_EEPROM_ADDRESS_1       64 

#define cclk_kHz  (SystemCoreClock / 1000);

unsigned int    IAP_command[ 5 ];
unsigned int    IAP_result[ 5 ];

/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

enum command_code {
    IAPCommand_Prepare_sector_for_write_operation    = 50,
    IAPCommand_Copy_RAM_to_Flash,
    IAPCommand_Erase_sector,
    IAPCommand_Blank_check_sector,
    IAPCommand_Read_part_ID,
    IAPCommand_Read_Boot_Code_version,
    IAPCommand_Compare,
    IAPCommand_Reinvoke_ISP,
    IAPCommand_Read_device_serial_number,
#if defined(TARGET_LPC11UXX)
    IAPCommand_EEPROM_Write = 61,
    IAPCommand_EEPROM_Read,
#elif defined(TARGET_LPC81X) || defined(TARGET_LPC82X)
    IAPCommand_Erase_page = 59,
#endif
};

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

static void write_eeprom(char *calabration_value, char target_addr)
{
    IAP_command[0] = IAPCommand_EEPROM_Write;
    IAP_command[1] = (unsigned int)target_addr;        //  Destination EEPROM address where data bytes are to be written. This address should be a 256 byte boundary.
    IAP_command[2] = (unsigned int)calabration_value;  //  Source RAM address from which data bytes are to be read. This address should be a word boundary.
    IAP_command[3] = MEM_SIZE;                         //  Number of bytes to be written. Should be 256 | 512 | 1024 | 4096.
    IAP_command[4] = (int)cclk_kHz;                    //  CPU Clock Frequency (CCLK) in kHz.

    iap_entry( IAP_command, IAP_result);
}

static void read_eeprom(char target_addr, char *result_addr)
{
    IAP_command[0] = IAPCommand_EEPROM_Read;
    IAP_command[1] = (unsigned int)target_addr;    //  Source EEPROM address from which data bytes are to be read. This address should be a word boundary.
    IAP_command[2] = (unsigned int)result_addr;    //  Destination RAM address where data bytes are to be written. This address should be a 256 byte boundary.
    IAP_command[3] = MEM_SIZE;                     //  Number of bytes to be written. Should be 256 | 512 | 1024 | 4096.
    IAP_command[4] = (int)cclk_kHz;                //  CPU Clock Frequency (CCLK) in kHz.

    iap_entry( IAP_command, IAP_result );
}

static void all_leds(int color, int status)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; ++i)
        hw_led(i, color, status);
}

static void set_setting_leds(uint8_t setting)
{
    //clear prev colors
    for (int i = 0; i < 3; ++i)
        all_leds(i, LED_OFF);

    for (int i = 0; i < FOOTSWITCHES_COUNT; ++i)
    {
        //first one is different because of menu exit
        if (i == 0)
            hw_led_set(i, LED_G, LED_ON, 0, 0);  
        else if (i == setting)
            hw_led_set(i, LED_W, LED_ON, 0, 0);
        else 
            hw_led_set(i, LED_R, LED_ON, 0, 0);
    }
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

uint32_t setting_get(uint8_t setting_id)
{
    //not used atm
    (void)setting_id;
    char MSG_BFR[MEM_SIZE];

    read_eeprom(TARGET_EEPROM_ADDRESS_1, MSG_BFR);

    return atoi(MSG_BFR);
}

void setting_set(uint8_t setting_id, uint32_t value)
{
    //not used atm
    (void)setting_id;

    char RAM_MSG[MEM_SIZE];

    char str_bfr[6];
    int_to_str(value, str_bfr, sizeof(str_bfr), 1, 0);

    RAM_MSG[0] = str_bfr[0];

    write_eeprom(RAM_MSG, TARGET_EEPROM_ADDRESS_1);
}

void settings_screen_run(void)
{
    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    // ask user to connect cable...
    clcd_cursor_set(0, CLCD_LINE1, 0);
    clcd_print(0, "SETTINGS: pages ");
    clcd_cursor_set(0, CLCD_LINE2, 0);
    clcd_print(0, "1 page 4 buttons");

    // ... and test the switches
    clcd_cursor_set(1, CLCD_LINE1, 0);
    clcd_print(1, "2 pages         ");
    clcd_cursor_set(1, CLCD_LINE2, 0);
    clcd_print(1, "3 pages         ");

    uint32_t page_setting =  setting_get(PAGE_SETTING_ID);

    set_setting_leds(page_setting);

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
                //set setting
                else 
                {
                    page_setting = i;
                    setting_set(PAGE_SETTING_ID, page_setting);
                    set_setting_leds(page_setting);
                }
            }
        }
        delay_ms(5);
    }
}

