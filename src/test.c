/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "hardware.h"
#include "serial.h"
#include "clcd.h"
#include "control_chain.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define FOOTSWITCHES_COUNT  4

#define CLEAR_LINE          "                "


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

static serial_t *g_serial;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static void all_leds(int color, int status)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; ++i)
        hw_led(i, color, status);
}

static void serial_recv(void *arg)
{
    static char buffer[24];
    static int index;

    cc_data_t *msg = arg;

    for (int i = 0; i < msg->size; i++)
    {
        buffer[index++] = msg->data[i];

        if (msg->data[i] == 0)
        {
            clcd_cursor_set(0, CLCD_LINE1, 0);
            clcd_print(0, buffer);
            index = 0;
        }
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

int main(void)
{
    hw_init();

    g_serial = serial_init(CC_BAUD_RATE, serial_recv);

    // message display 1
    clcd_cursor_set(0, CLCD_LINE1, 0);
    clcd_print(0, "MOD DEVICES");
    clcd_cursor_set(0, CLCD_LINE2, 0);
    clcd_print(0, "CONTROL CHAIN");

    // message display 2
    clcd_cursor_set(1, CLCD_LINE1, 0);
    clcd_print(1, "FOOTSWITCH EXT.");
    clcd_cursor_set(1, CLCD_LINE2, 0);
    clcd_print(1, "FW VER: " CC_FIRMWARE_VERSION);

    delay_ms(1000);

    // test red leds
    all_leds(LED_R, LED_ON);
    delay_ms(1000);
    all_leds(LED_R, LED_OFF);

    // test green leds
    all_leds(LED_G, LED_ON);
    delay_ms(1000);
    all_leds(LED_G, LED_OFF);

    // test blue leds
    all_leds(LED_B, LED_ON);
    delay_ms(1000);
    all_leds(LED_B, LED_OFF);

    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    // ask user to connect cable...
    clcd_cursor_set(0, CLCD_LINE1, 0);
    clcd_print(0, "PLUG CROSSOVER  ");
    clcd_cursor_set(0, CLCD_LINE2, 0);
    clcd_print(0, "PATCH CABLE     ");

    // ... and test the switches
    clcd_cursor_set(1, CLCD_LINE1, 0);
    clcd_print(1, "PRESS DOWN ALL  ");
    clcd_cursor_set(1, CLCD_LINE2, 0);
    clcd_print(1, "FOOTSWITCHES    ");

    static int state[FOOTSWITCHES_COUNT];
    static serial_data_t msg;
    msg.data = (uint8_t *) "COMMUNICATION OK";
    msg.size = 17;

    while (1)
    {
        // test buttons
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                state[i] = 1 - state[i];
                hw_led(i, LED_R, state[i]);
            }
        }

        serial_send(g_serial, &msg);
        delay_ms(5);
    }

    return 0;
}
