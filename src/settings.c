/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdlib.h>
#include "hardware.h"
#include "clcd.h"
#include "chip.h"
#include "config.h"
#include "utils.h"
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
    IAPCommand_EEPROM_Write = 61,
    IAPCommand_EEPROM_Read
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

void write_eeprom(char *value, char target_addr)
{;
    IAP_command[0] = IAPCommand_EEPROM_Write;
    IAP_command[1] = (unsigned int)target_addr;        //  Destination EEPROM address where data bytes are to be written. This address should be a 256 byte boundary.
    IAP_command[2] = (unsigned int)value;              //  Source RAM address from which data bytes are to be read. This address should be a word boundary.
    IAP_command[3] = 1;                         //  Number of bytes to be written. Should be 256 | 512 | 1024 | 4096.
    IAP_command[4] = (int)cclk_kHz;                    //  CPU Clock Frequency (CCLK) in kHz.
 
    iap_entry( IAP_command, IAP_result);
}

void read_eeprom(char target_addr, char *result_addr)
{;
    IAP_command[0] = IAPCommand_EEPROM_Read;
    IAP_command[1] = (unsigned int)target_addr;    //  Source EEPROM address from which data bytes are to be read. This address should be a word boundary.
    IAP_command[2] = (unsigned int)result_addr;    //  Destination RAM address where data bytes are to be written. This address should be a 256 byte boundary.
    IAP_command[3] = 1;                     //  Number of bytes to be written. Should be 256 | 512 | 1024 | 4096.
    IAP_command[4] = (int)cclk_kHz;                //  CPU Clock Frequency (CCLK) in kHz.
 
    iap_entry( IAP_command, IAP_result );
}

void setting_set(uint8_t setting_id, uint32_t value)
{
    char MSG_BFR[6];
    
    int_to_str(value, MSG_BFR, sizeof(MSG_BFR), 1, 0);

    write_eeprom(MSG_BFR, setting_id);
}


void write_screen_settings(int pages, int chainID)
{
    char value_str_bfr[6];

    clcd_cursor_set(1, CLCD_LINE1, 0);
    clcd_print(1, "Pages:");
    int_to_str(pages, value_str_bfr, sizeof(value_str_bfr), 1, 0);
    clcd_cursor_set(1, CLCD_LINE1, 15);
    clcd_print(1, value_str_bfr);

    clcd_cursor_set(1, CLCD_LINE2, 0);
    clcd_print(1, "Chain ID:");
    int_to_str(chainID, value_str_bfr, sizeof(value_str_bfr), 1, 0);
    clcd_cursor_set(1, CLCD_LINE2, 15);
    clcd_print(1, value_str_bfr);
}

/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

int setting_get(uint8_t setting_id)
{
    char MSG_BFR[6];

    read_eeprom(setting_id, MSG_BFR);

    return atoi(MSG_BFR);
}

void settings_screen_run(void)
{
    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    clcd_cursor_set(0, CLCD_LINE1, 0);
    clcd_print(0, "SETTING MODE");
    clcd_cursor_set(0, CLCD_LINE2, 0);
    clcd_print(0, "Foot 1 to save");
    hw_led_set(0, LED_G, LED_ON, 0, 0);

    //get the saved settings to display
    uint32_t page_setting = setting_get(PAGE_SETTING_ID);
    uint32_t chain_id = setting_get(CC_CHAIN_ID_ID);

    write_screen_settings(page_setting, chain_id);

    hw_led_set(2, LED_W, LED_ON, 0, 0);
    hw_led_set(3, LED_W, LED_ON, 0, 0);

    while (1)
    {
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                //first button means exit (or in the furute next menu item)
                if (i == 0)
                {
                    setting_set(PAGE_SETTING_ID, page_setting);
                    setting_set(CC_CHAIN_ID_ID, chain_id);

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

                    for (uint8_t q = 0; q < FOOTSWITCHES_COUNT; q++) {
                        hw_led_set(q, LED_W, LED_OFF, 0, 0);
                        hw_led_set(q, LED_G, LED_ON, 0, 0);
                    }

                    //lock the device till reboot
                    while (1); 
                }
                //set setting
                else if (i == 2)
                {
                    if (page_setting < 3)
                        page_setting++;
                    else
                        page_setting = 1;

                    write_screen_settings(page_setting, chain_id);
                }
                //set setting
                else if (i == 3)
                {
                    if (chain_id < 8)
                        chain_id++;
                    else
                        chain_id = 1;

                    write_screen_settings(page_setting, chain_id);
                }
            }
        }
        delay_ms(5);
    }
}