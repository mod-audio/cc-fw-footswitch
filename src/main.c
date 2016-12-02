/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "hardware.h"
#include "serial.h"
#include "control_chain.h"
#include "actuator.h"
#include "update.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define BAUD_RATE 115200


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
    cc_data_t *data = arg;
    cc_parse(data);
}

static void response_cb(void *arg)
{
    serial_data_t *data = arg;
    serial_send(g_serial, data);
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

int main(void)
{
    hw_init();
    g_serial = serial_init(BAUD_RATE, serial_recv);
    cc_init(response_cb);

    static volatile float foots[4];

    for (int i = 0; i < 4; i++)
    {
        cc_actuator_new(&foots[i]);
    }

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                foots[i] = 1.0;
            }
            else if (button_status == BUTTON_RELEASED)
            {
                foots[i] = 0.0;
            }
        }

        cc_process();

        for (int i = 0; i < 4; i++)
            hw_led(i, LED_R, LED_OFF);

        cc_assignments_t *assignments = cc_assignments();
        if (assignments)
        {
            LILI_FOREACH(assignments, node)
            {
                cc_assignment_t *assignment = node->data;
                if (assignment->mode == CC_MODE_TOGGLE)
                    hw_led(assignment->actuator_id, LED_R, assignment->value ? LED_ON : LED_OFF);
            }
        }
    }

    return 0;
}
