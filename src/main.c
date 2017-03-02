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
static float g_foot_value[FOOTSWITCHES_COUNT];


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static void waiting_message(int foot)
{
    char text[] = {"FOOT #X"};

    // foot number
    text[6] = '1' + foot;

    // define position to print
    int lcd = (foot & 0x02) >> 1;
    int line = foot & 0x01;

    // print message
    clcd_cursor_set(lcd, line, 0);
    clcd_print(lcd, text);
}

static void welcome_message(void)
{
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

    // wait until user to see the message
    delay_ms(3000);

    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    // print waiting message for all footswitches
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        waiting_message(i);
}

static void turn_off_leds(void)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        hw_led(i, LED_R, LED_OFF);
        hw_led(i, LED_G, LED_OFF);
        hw_led(i, LED_B, LED_OFF);
    }
}

static void update_leds(cc_assignment_t *assignment)
{
    if (assignment->mode == CC_MODE_TOGGLE)
        hw_led(assignment->actuator_id, LED_R, assignment->value ? LED_ON : LED_OFF);
    else if (assignment->mode == CC_MODE_TRIGGER)
        hw_led(assignment->actuator_id, LED_G, LED_ON);
}

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

static void events_cb(void *arg)
{
    cc_event_t *event = arg;

    if (event->id == CC_EV_HANDSHAKE_FAILED ||
        event->id == CC_EV_DEVICE_DISABLED)
    {
        int *status = event->data;
        if (*status == CC_UPDATE_REQUIRED)
        {
            // clear displays and leds
            turn_off_leds();
            clcd_clear(0);
            clcd_clear(1);

            // show update message
            clcd_cursor_set(0, 0, 0);
            clcd_print(0, "This device need");
            clcd_cursor_set(0, 1, 0);
            clcd_print(0, "to be updated");

            // FIXME: not cool, not cool
            while (1);
        }
    }
    else if (event->id == CC_EV_ASSIGNMENT)
    {
        cc_assignment_t *assignment = event->data;

        int lcd = (assignment->actuator_id & 0x02) >> 1;
        int line = assignment->actuator_id & 0x01;

        // clear lcd line
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, CLEAR_LINE);

        // print assignment label
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, assignment->label.text);

        update_leds(assignment);
    }
    else if (event->id == CC_EV_UNASSIGNMENT)
    {
        int *act_id = event->data;
        int actuator_id = *act_id;

        int lcd = (actuator_id & 0x02) >> 1;
        int line = actuator_id & 0x01;

        // clear lcd line
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, CLEAR_LINE);

        waiting_message(actuator_id);

        // turn off leds
        hw_led(actuator_id, LED_R, LED_OFF);
        hw_led(actuator_id, LED_G, LED_OFF);
        hw_led(actuator_id, LED_B, LED_OFF);
    }
    else if (event->id == CC_EV_UPDATE)
    {
        cc_assignment_t *assignment = event->data;
        update_leds(assignment);
    }
    else if (event->id == CC_EV_MASTER_RESETED)
    {
        // clear displays and leds
        clcd_clear(0);
        clcd_clear(1);
        turn_off_leds();

        // print waiting message for all footswitches
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
            waiting_message(i);
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
    welcome_message();

    // init and create device
    cc_init(response_cb, events_cb);
    cc_device_t *device = cc_device_new("FootEx", "https://github.com/moddevices/cc-fw-footswitch");

    // create actuators
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        char name[16] = {"Foot #"};
        name[6] = '1' + i;
        name[7] = 0;

        cc_actuator_config_t actuator_config;
        actuator_config.type = CC_ACTUATOR_MOMENTARY;
        actuator_config.name = name;
        actuator_config.value = &g_foot_value[i];
        actuator_config.min = 0.0;
        actuator_config.max = 1.0;
        actuator_config.supported_modes = CC_MODE_TOGGLE | CC_MODE_TRIGGER;
        actuator_config.max_assignments = 1;

        cc_actuator_t *actuator = cc_actuator_new(&actuator_config);
        cc_device_actuator_add(device, actuator);
    }

    // init serial
    g_serial = serial_init(CC_BAUD_RATE, serial_recv);

    while (1)
    {
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
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
    }

    return 0;
}
