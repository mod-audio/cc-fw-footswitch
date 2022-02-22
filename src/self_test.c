/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "self_test.h"
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

static void serial_recv(void *arg)
{
    static char buffer[24];
    static int index, count, j;

    cc_data_t *msg = arg;

    for (uint32_t i = 0; i < msg->size; i++)
    {
        buffer[index++] = msg->data[i];

        if (msg->data[i] == 0)
        {
            clcd_cursor_set(0, CLCD_LINE1, 0);
            clcd_print(0, buffer);

            // animation
            if (count++ >= 30)
            {
                clcd_cursor_set(0, CLCD_LINE2, 0);
                clcd_print(0, CLEAR_LINE);

                clcd_cursor_set(0, CLCD_LINE2, j++);
                clcd_print(0, "*");

                if (j == 16)
                    j = 0;

                count = 0;
            }

            index = 0;
        }
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void self_test_run(void)
{
    g_serial = serial_init(CC_BAUD_RATE, serial_recv);

    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    // ask user to connect cable...
    clcd_cursor_set(0, CLCD_LINE1, 0);
    clcd_print(0, "SELFTEST: PLUG  ");
    clcd_cursor_set(0, CLCD_LINE2, 0);
    clcd_print(0, "CROSSOVER CABLE ");

    // ... and test the switches
    clcd_cursor_set(1, CLCD_LINE1, 0);
    clcd_print(1, "PRESS DOWN ALL  ");
    clcd_cursor_set(1, CLCD_LINE2, 0);
    clcd_print(1, "FOOTSWITCHES 4x ");

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
                state[i]++;

                if (state[i] == 1)
                {
                    hw_led_set(i, LED_R, 1,0,0);
                    hw_led_set(i, LED_G, 0,0,0);
                    hw_led_set(i, LED_B, 0,0,0);
                }
                else if (state[i] == 2)
                {
                    hw_led_set(i, LED_R, 0,0,0);
                    hw_led_set(i, LED_G, 1,0,0);
                    hw_led_set(i, LED_B, 0,0,0);
                }
                else if (state[i] == 3)
                {
                    hw_led_set(i, LED_R, 0,0,0);
                    hw_led_set(i, LED_G, 0,0,0);
                    hw_led_set(i, LED_B, 1,0,0);
                }
                else if (state[i] == 4)
                {
                    hw_led_set(i, LED_R, 0,0,0);
                    hw_led_set(i, LED_G, 0,0,0);
                    hw_led_set(i, LED_B, 0,0,0);
                    state[i] = 0;
                }
            }
        }

        serial_send(g_serial, &msg);
        delay_ms(5);
    }
}
