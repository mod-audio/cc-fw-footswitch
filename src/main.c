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

#define BAUD_RATE           115200
#define FOOTSWITCHES_COUNT  4


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
static float g_foot_value[FOOTSWITCHES_COUNT];


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
    cc_device_t *device = cc_device_new("FootEx", "uri:FootEx");

    // create actuators
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        cc_actuator_t *actuator =
            cc_actuator_new(CC_ACTUATOR_MOMENTARY, &g_foot_value[i], 0, 1);

        cc_device_actuator_add(device, actuator);
    }

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                g_foot_value[i] = 1.0;
            }
            else if (button_status == BUTTON_RELEASED)
            {
                g_foot_value[i] = 0.0;
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
