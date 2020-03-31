/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "hardware.h"
#include "serial.h"
#include "clcd.h"
#include "control_chain.h"
#include "config.h"
#include "self_test.h"
#include "settings.h"

#include <string.h>
#include <math.h>

/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

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

enum {TT_INIT, TT_COUNTING};

struct TAP_TEMPO_T {
    uint32_t time, max;
    uint8_t state;
};

/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static serial_t *g_serial;
static float g_foot_value[MAX_PAGES][FOOTSWITCHES_COUNT];
static unsigned int g_welcome_timeout = 10; //1000000;
static uint8_t g_current_page = 0;
static uint8_t g_pages_count = 0;
static uint8_t g_foots_assigned[MAX_PAGES * FOOTSWITCHES_COUNT];
static struct TAP_TEMPO_T g_tap_tempo[MAX_PAGES * FOOTSWITCHES_COUNT];

/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static float convert_to_ms(const char *unit_from, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_from[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_from[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0f / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0f / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value * 1000.0f);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0f;
}

static float convert_from_ms(const char *unit_to, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_to[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_to[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0f / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0f / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value / 1000.0f);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0f;
}

static void handle_tap_tempo(uint8_t actuator_id)
{
    uint32_t now = hw_uptime();
    uint32_t delta = now - g_tap_tempo[actuator_id + (3*g_current_page)].time;
    g_tap_tempo[actuator_id + (3*g_current_page)].time = now;

    cc_assignment_t *assignment = cc_assignment_get(g_foots_assigned[actuator_id + (g_current_page * (FOOTSWITCHES_COUNT-1))]);

    // checks if delta almost suits maximum allowed value
    if ((delta > g_tap_tempo[actuator_id + (3*g_current_page)].max) &&
        ((delta - TAP_TEMPO_MAXVAL_OVERFLOW) < g_tap_tempo[actuator_id + (3*g_current_page)].max))
    {
        // sets delta to maxvalue if just slightly over, instead of doing nothing
        delta = g_tap_tempo[actuator_id + (3*g_current_page)].max;
    }

    // checks the tap tempo timeout
    if (delta <= g_tap_tempo[actuator_id + (3*g_current_page)].max)
    {
        //get current value of tap tempo in ms
        float currentTapVal = convert_to_ms(assignment->unit.text, assignment->value);
        //check if it should be added to running average
        float tmp_tempo = 0;
        if (fabs(currentTapVal - delta) < TAP_TEMPO_TAP_HYSTERESIS)
        {
            // converts and update the tap tempo value
            tmp_tempo = (2*(assignment->value) + convert_from_ms(assignment->unit.text, delta)) / 3;
        }
        else
        {
            // converts and update the tap tempo value
            tmp_tempo = convert_from_ms(assignment->unit.text, delta);
        }

        // checks the values bounds
        if (tmp_tempo > assignment->max) tmp_tempo = assignment->max;
        else if (tmp_tempo < assignment->min) tmp_tempo = assignment->min;

        g_foot_value[g_current_page][actuator_id] = tmp_tempo;
    }
}

static uint8_t get_actuator_page(int actuator_id)
{
    if (actuator_id < 1*(FOOTSWITCHES_COUNT-1))
        return 0;
    else if (actuator_id < 2*(FOOTSWITCHES_COUNT-1))
        return 1;
    else if (actuator_id < 3*(FOOTSWITCHES_COUNT-1))
        return 2;
    else 
        return 0;
}

static void waiting_message(int foot)
{
    if ((g_pages_count != 1) && (foot == 0))
    {
        char text[] = {"PAGE #X/Y"};
        // foot number
        text[6] = '1' + g_current_page;
        text[8] = '1' + (g_pages_count-1);
        text[9] = 0;

        // define position to print
        int lcd = (foot & 0x02) >> 1;
        int line = foot & 0x01;

        // print message
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, text);

        hw_led_set(foot, LED_W, LED_ON, 0,0);
    }
    else if (g_pages_count == 1)
    {
        char text[17] = {"FOOT #X"};
        // foot number
        text[6] = '1' + foot;
        text[7] = 0;

        // define position to print
        int lcd = (foot & 0x02) >> 1;
        int line = foot & 0x01;

        // print message
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, text);
    }
    else 
    {
        char text[17] = {"FOOT P#Y #X"};
        // foot number
        text[7] = '1' + g_current_page;
        text[10] = '1' + (foot-1);
        text[11] = 0;

        // define position to print
        int lcd = (foot & 0x02) >> 1;
        int line = foot & 0x01;

        // print message
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, text);
    }
}

static void turn_off_leds(void)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        hw_led_set(i, LED_R, LED_OFF, 0, 0);
        hw_led_set(i, LED_G, LED_OFF, 0, 0);
        hw_led_set(i, LED_B, LED_OFF, 0, 0);
    }
}

static void clear(int8_t actuator)
{
    //clear all
    if (actuator == -1)
    {
        turn_off_leds();

        // clear displays
        clcd_clear(0);
        clcd_clear(1);

        // print waiting message for all footswitches
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
            waiting_message(i);
    }
    //induvidual clear
    else
    {
        if (g_pages_count != 1)
        {
            actuator++;
        }

        int lcd = 0;
        int line = 0;

        lcd = (actuator & 0x02) >> 1;
        line = actuator & 0x01;
        hw_led_set(actuator, LED_W, LED_OFF, 0, 0);

        char buffer[17] = CLEAR_LINE;

        // print buffer to lcd
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, buffer);

        waiting_message(actuator);
    }
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

    if (hw_self_test())
        clcd_print(1, "SELF-TEST MODE");
    else if (hw_settings_mode())
    	clcd_print(1, "SETTINGS MODE");
    else
        clcd_print(1, "FW VER: " CC_FIRMWARE_VERSION);
}

static void update_leds(cc_assignment_t *assignment)
{
    if (g_pages_count == 1)
    {
        if (assignment->mode & (CC_MODE_TRIGGER | CC_MODE_OPTIONS))
            hw_led_set(assignment->actuator_id, LED_G, LED_ON, 0, 0);
        else if (assignment->mode & CC_MODE_TOGGLE)
            hw_led_set(assignment->actuator_id, LED_R, assignment->value ? LED_ON : LED_OFF, 0, 0);
        else if (assignment->mode & CC_MODE_TAP_TEMPO)
            hw_led_set(assignment->actuator_id, LED_G, LED_ON, TAP_TEMPO_TIME_ON, (convert_to_ms(assignment->unit.text, assignment->value) - TAP_TEMPO_TIME_ON));
    }
    else 
    {
        if (get_actuator_page(assignment->actuator_id) == g_current_page)
        {
            if (assignment->mode & (CC_MODE_TRIGGER | CC_MODE_OPTIONS))
                hw_led_set(((assignment->actuator_id + 1) - (g_current_page*(FOOTSWITCHES_COUNT-1))), LED_G, LED_ON, 0, 0);
            else if (assignment->mode & CC_MODE_TOGGLE)
                hw_led_set(((assignment->actuator_id + 1) - (g_current_page*(FOOTSWITCHES_COUNT-1))), LED_R, assignment->value ? LED_ON : LED_OFF, 0, 0);
            else if (assignment->mode & CC_MODE_TAP_TEMPO)
                hw_led_set(((assignment->actuator_id + 1) - (g_current_page*(FOOTSWITCHES_COUNT-1))), LED_G, LED_ON, TAP_TEMPO_TIME_ON, (convert_to_ms(assignment->unit.text, assignment->value) - TAP_TEMPO_TIME_ON));
        }
    }
}

static void update_lcds(cc_assignment_t *assignment)
{
    int lcd = 0;
    int line = 0;

    if (g_pages_count == 1)
    {
        lcd = (assignment->actuator_id & 0x02) >> 1;
        line = assignment->actuator_id & 0x01;
    }
    else if (get_actuator_page(assignment->actuator_id) == g_current_page)
    {
        lcd = (((assignment->actuator_id + 1)- (g_current_page*(FOOTSWITCHES_COUNT-1)))& 0x02) >> 1;
        line = ((assignment->actuator_id + 1)- (g_current_page*(FOOTSWITCHES_COUNT-1)))& 0x01;
    }	
    else
    {
        return;
    }

    char buffer[17];
    int i;

    // init buffer with spaces
    for (i = 0; i < sizeof(buffer) - 1; i++)
        buffer[i] = ' ';

    // copy assignment label
    if (assignment->label.size == 0)
    {
        waiting_message((assignment->actuator_id + (g_pages_count==1? 0:1))- (g_current_page*(FOOTSWITCHES_COUNT-1)));
        return;
    }
    else
    {
        for (i = 0; i < assignment->label.size; i++)
            buffer[i] = assignment->label.text[i];
    }

    // copy item label if it's option mode
    if (assignment->mode & CC_MODE_OPTIONS)
    {
        // separator
        buffer[i++] = ':';

        // copy item label
        str16_t *item_label = &assignment->list_items[assignment->list_index]->label;
        for (int j = 0; j < item_label->size && i < sizeof(buffer); j++, i++)
            buffer[i] = item_label->text[j];
    }
    else if (assignment->mode & CC_MODE_TAP_TEMPO)
    {
        // separator
        buffer[i++] = ':';
        buffer[i++] = ' ';

        //print as int of float
        //s with 2 decimals
        if (strcmp(assignment->unit.text, "s") == 0 || strcmp(assignment->unit.text, "hz") == 0)
        {
            // copy value to label
            char value_label[6];
            uint8_t value_size = float_to_str(assignment->value, value_label, sizeof(value_label),2);
            for (int j = 0; j < value_size && i < sizeof(buffer); j++, i++)
                buffer[i] = value_label[j];
        }
        //bpm and ms as int
        else
        {
            // copy value to label
            char value_label[6];
            uint8_t value_size = int_to_str(assignment->value, value_label, sizeof(value_label),0,0);
            for (int j = 0; j < value_size && i < sizeof(buffer); j++, i++)
                buffer[i] = value_label[j];
        }
        buffer[i++] = ' ';

        // copy unit label
        str16_t *item_label = &assignment->unit;
        for (int j = 0; j < item_label->size && i < sizeof(buffer); j++, i++)
            buffer[i] = item_label->text[j];
    }

    // make buffer null-terminated
    buffer[sizeof(buffer) - 1] = 0;

    // print buffer to lcd
    clcd_cursor_set(lcd, line, 0);
    clcd_print(lcd, buffer);
}

static void set_actuator_page(int page_id)
{
    //clear all screens
    clear (-1);

    //print pages message
    waiting_message(0);

    for (uint8_t i = 0; i < FOOTSWITCHES_COUNT-1; i++)
    {
        if (g_foots_assigned[i + (page_id * (FOOTSWITCHES_COUNT-1))] != -1)
        {
            update_leds(cc_assignment_get(g_foots_assigned[i + (page_id * (FOOTSWITCHES_COUNT-1))]));
            update_lcds(cc_assignment_get(g_foots_assigned[i + (page_id * (FOOTSWITCHES_COUNT-1))]));
        }
    }
}

static void serial_recv(void *arg)
{
    cc_data_t *data = arg;

    // use regular baud rate option if parse failed many times
    if (cc_parse(data) < 0)
        serial_baud_rate_set(CC_BAUD_RATE);
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
            clcd_print(0, "This device needs");
            clcd_cursor_set(0, 1, 0);
            clcd_print(0, "to be updated");

            // FIXME: not cool, not cool
            while (1);
        }
    }
    else if (event->id == CC_EV_ASSIGNMENT)
    {
        // force cleaning if still on welcome message
        if (g_welcome_timeout > 0)
        {
            g_welcome_timeout = 0;
            clear(-1);
        }

        int *act_id = event->data;
        int actuator_id = *act_id;

        cc_assignment_t *assignment = cc_assignment_get(actuator_id);

        g_foots_assigned[assignment->actuator_id] = actuator_id; 

        if (assignment->mode & CC_MODE_TAP_TEMPO)
        {
            // calculates the maximum tap tempo value
            if (g_tap_tempo[assignment->actuator_id].state == TT_INIT)
            {
                uint32_t max;

                // time unit (ms, s)
                if (strcmp(assignment->unit.text, "ms") == 0 || strcmp(assignment->unit.text, "s") == 0)
                {
                    max = (uint32_t)(convert_to_ms(assignment->unit.text, assignment->max) + 0.5);
                    //makes sure we enforce a proper timeout
                    if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                }
                // frequency unit (bpm, Hz)
                else
                {
                    //prevent division by 0 case
                    if (assignment->min == 0)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    else
                        max = (uint32_t)(convert_to_ms(assignment->unit.text, assignment->min) + 0.5);

                    //makes sure we enforce a proper timeout
                    if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                        max = TAP_TEMPO_DEFAULT_TIMEOUT;
                }

                g_tap_tempo[assignment->actuator_id].max = max;
                g_tap_tempo[assignment->actuator_id].state = TT_COUNTING;
            }
        }

        update_leds(assignment);
        update_lcds(assignment);
    }
    else if (event->id == CC_EV_UNASSIGNMENT)
    {
        int *act_id = event->data;
        int actuator_id = *act_id;
        cc_assignment_t *assignment = cc_assignment_get(actuator_id);

        //only clear screen if on active page
        if ((g_current_page == get_actuator_page(actuator_id)) || (g_pages_count == 1))
        {
            clear(actuator_id - (g_current_page * (FOOTSWITCHES_COUNT-1)));
        }

        g_foots_assigned[assignment->actuator_id] = -1; 
        g_tap_tempo[assignment->actuator_id].state = TT_INIT;
    }
    else if (event->id == CC_EV_UPDATE)
    {
        cc_assignment_t *assignment = event->data;

        if ((g_current_page == get_actuator_page(assignment->actuator_id)) || (g_pages_count == 1))
        {
            update_leds(assignment);
            update_lcds(assignment);
        }
    }
    else if (event->id == CC_CMD_SET_VALUE)
    {
        cc_set_value_t *set_value = event->data;
        cc_assignment_t *assignment = cc_assignment_get(set_value->actuator_id);
        assignment->value = set_value->value;

        if ((g_current_page == get_actuator_page(set_value->actuator_id)) || (g_pages_count == 1))
        {
            update_leds(assignment);
            update_lcds(assignment);
        }
    }
    else if (event->id == CC_EV_MASTER_RESETED)
    {
        clear(-1);
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

    // execute self-test if required
    // the device never leaves the self-test routine
    if (hw_self_test())
        self_test_run();
    else if (hw_settings_mode())
        settings_screen_run();

    for (uint8_t j = 0; j < (MAX_PAGES * FOOTSWITCHES_COUNT); j++)
    {
        g_foots_assigned[j] = -1;
        g_tap_tempo[j].state = TT_INIT;
    }

    //get the current page setting (the only setting for now)
    g_pages_count = setting_get(PAGE_SETTING_ID);
    //if out of bounds, resort to default and write to eeprom too.
    if ((g_pages_count < 1) || (g_pages_count > MAX_PAGES))
    {
    	g_pages_count = 1;
    	setting_set(PAGE_SETTING_ID, g_pages_count);
    }

    // init and create device
    cc_init(response_cb, events_cb);
    cc_device_t *device = cc_device_new("FootEx", "https://github.com/moddevices/cc-fw-footswitch");

    if (g_pages_count == 1)
    {
        // create actuators
        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            char name[16] = {"Foot #"};
            name[6] = '1' + i;
            name[7] = 0;

            cc_actuator_config_t actuator_config;
            actuator_config.type = CC_ACTUATOR_MOMENTARY;
            actuator_config.name = name;
            actuator_config.value = &g_foot_value[0][i];
            actuator_config.min = 0.0;
            actuator_config.max = 1.0;
            actuator_config.supported_modes = CC_MODE_TOGGLE | CC_MODE_TRIGGER | CC_MODE_OPTIONS;
            actuator_config.max_assignments = 1;

            cc_actuator_t *actuator = cc_actuator_new(&actuator_config);
            cc_device_actuator_add(device, actuator);
        }
    }
    else 
    {
        // create actuators
        for (int j = 0; j < (g_pages_count); j++)
        {
            // create actuators
            for (int i = 0; i < (FOOTSWITCHES_COUNT-1); i++)
            {
                char name[16] = {"Foot P"};
                name[6] = '1' + (j);
                name[7] = ' ';
                name[8] = '#';
                name[9] = '1' + (i);
                name[10] = 0;

                cc_actuator_config_t actuator_config;
                actuator_config.type = CC_ACTUATOR_MOMENTARY;
                actuator_config.name = name;
                actuator_config.value = &g_foot_value[j][i];
                actuator_config.min = 0.0;
                actuator_config.max = 1.0;
                actuator_config.supported_modes = CC_MODE_TOGGLE | CC_MODE_TRIGGER | CC_MODE_OPTIONS | CC_MODE_TAP_TEMPO;
                actuator_config.max_assignments = 1;

                cc_actuator_t *actuator = cc_actuator_new(&actuator_config);
                cc_device_actuator_add(device, actuator);
            }
        }
    }

    // init serial
    g_serial = serial_init(CC_BAUD_RATE_FALLBACK, serial_recv);

    //prev state
    uint8_t prev_footswitch_state[FOOTSWITCHES_COUNT] = {};

    while (1)
    {

        if (g_welcome_timeout > 0)
        {
            if (--g_welcome_timeout == 0)
                clear(-1);
        }

        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            int button_status = hw_button(i);
            if (g_pages_count == 1)
            {
                if (button_status == BUTTON_PRESSED)
                {
                    if (g_tap_tempo[i].state == TT_COUNTING)
                    {
                        //handle tap tempo
                        handle_tap_tempo(i);
                    }
                    else 
                    {
                        g_foot_value[0][i] = 1.0;
                    }
                }
                else if (button_status == BUTTON_RELEASED)
                {
                    if (g_tap_tempo[i].state != TT_COUNTING)
                    {
                       g_foot_value[0][i] = 0.0;
                    }
                }
            }
            else 
            {
                if ((i == 0) && (button_status == BUTTON_PRESSED))
                {
                    //switch to next page
                    if (g_current_page < g_pages_count-1)
                        g_current_page++;
                    else 
                        g_current_page = 0;

                    //update leds and stuff
                    set_actuator_page(g_current_page);

                    //make LED red
                    hw_led_set(i, LED_W, LED_OFF, 0, 0);
                    hw_led_set(i, LED_R, LED_ON, 0, 0); 

                    prev_footswitch_state[i] = 1;

                }
                else if ((i == 0) && (button_status == BUTTON_RELEASED))
                {
                    if (prev_footswitch_state[i])
                    {
                        hw_led_set(i, LED_W, LED_ON, 0, 0);
                        prev_footswitch_state[i] = 0;		
                    }
                }
                else 
                {
                    if (button_status == BUTTON_PRESSED)
                    {
                        g_foot_value[g_current_page][i-1] = 1.0;

                        if (g_tap_tempo[(i-1) + (3*g_current_page)].state == TT_COUNTING)
                        {
                            //handle tap tempo
                            handle_tap_tempo(i-1);
                        }
                        else 
                        {
                            g_foot_value[g_current_page][i-1] = 1.0;
                        }
                    }
                    else if (button_status == BUTTON_RELEASED)
                    {
                        if (g_tap_tempo[(i-1) + (3*g_current_page)].state != TT_COUNTING)
                        {
                           g_foot_value[g_current_page][i-1] = 0.0;
                        }
                    }
                }
            }
        }

        cc_process();
    }

    return 0;
}
