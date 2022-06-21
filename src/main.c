/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "hardware.h"
#include "serial.h"
#include "clcd.h"
#include "control_chain.h"
#include "self_test.h"
#include "settings.h"
#include "config.h"
#include "util.h"
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
static float g_foot_value[FOOTSWITCHES_COUNT];
static unsigned int g_welcome_timeout = 200000;
static struct TAP_TEMPO_T g_tap_tempo[FOOTSWITCHES_COUNT] = {};
static cc_assignment_t *g_current_assignment[FOOTSWITCHES_COUNT];
static int g_pages_amount = 1, g_current_page = 1;

/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static float convert_ms(const char *unit, float value, uint8_t to_ms)
{
    char unit_lowercase[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit[i] && i < (sizeof(unit_lowercase)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit_lowercase[i] = unit[i] | 0x20;
    }
    unit_lowercase[i] = 0;

    if (strcmp(unit_lowercase, "bpm") == 0)
    {
        return (60000.0f / value);
    }
    else if (strcmp(unit_lowercase, "hz") == 0)
    {
        return (1000.0f / value);
    }
    else if (strcmp(unit_lowercase, "s") == 0)
    {
        if (to_ms)
            return (value * 1000.0f);
        else
            return (value / 1000.0f);
    }

    return value;
}

static void print_page_number(void)
{
    char text[] = {"PAGE: X / Y"};

    // foot number
    text[6] = '0' + g_current_page;

    text[10] = '0' + g_pages_amount;

    // print message
    clcd_cursor_set(0, 0, 0);
    clcd_print(0, text);
}

static void handle_tap_tempo(uint8_t actuator_id)
{
    uint32_t now = hw_uptime();
    uint32_t delta = now - g_tap_tempo[actuator_id].time;
    g_tap_tempo[actuator_id].time = now;

    cc_assignment_t *assignment = g_current_assignment[actuator_id];

    // checks if delta almost suits maximum allowed value
    if ((delta > g_tap_tempo[actuator_id ].max) &&
        ((delta - TAP_TEMPO_MAXVAL_OVERFLOW) < g_tap_tempo[actuator_id].max))
    {
        // sets delta to maxvalue if just slightly over, instead of doing nothing
        delta = g_tap_tempo[actuator_id].max;
    }

    // checks the tap tempo timeout
    if (delta <= g_tap_tempo[actuator_id].max)
    {
        //get current value of tap tempo in ms
        float currentTapVal = convert_ms(assignment->unit.text, assignment->value, 1);
        //check if it should be added to running average
        float tmp_tempo = 0;
        if (fabs(currentTapVal - delta) < TAP_TEMPO_TAP_HYSTERESIS)
        {
            // converts and update the tap tempo value
            tmp_tempo = (2*(assignment->value) + convert_ms(assignment->unit.text, delta, 0)) / 3;
        }
        else
        {
            // converts and update the tap tempo value
            tmp_tempo = convert_ms(assignment->unit.text, delta, 0);
        }

        //float tmp_tempo = delta;

        // checks the values bounds
        if (tmp_tempo > assignment->max) tmp_tempo = assignment->max;
        else if (tmp_tempo < assignment->min) tmp_tempo = assignment->min;

        g_foot_value[actuator_id] = tmp_tempo;
    }
}

static void waiting_message(int foot)
{
    int lcd = (foot & 0x02) >> 1;
    int line = foot & 0x01;

    if ((g_pages_amount > 1) && (foot == 0)) {
        print_page_number();
    }

    else {
        char text[] = {"FOOT #X"};

        // foot number
        if (g_pages_amount > 1)
            text[6] = '0' + foot;
        else
            text[6] = '1' + foot;

        // print message
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, text);
    }
}

static void welcome_message(uint8_t boot_mode)
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

    if (boot_mode == BOOT_SELFTEST)
        clcd_print(1, "SELF-TEST MODE");
    else
        clcd_print(1, "FW VER: " CC_FIRMWARE_VERSION);
}

static void lcd_group_widget(cc_assignment_t *assignment)
{
    uint8_t id = assignment->actuator_id;
    if (g_pages_amount > 1)
        id++;

    int lcd = (id & 0x02) >> 1;

    char buffer[17];
    uint8_t i = 0;

    // init buffer with spaces
    for (i = 0; i < sizeof(buffer) - 1; i++)
        buffer[i] = ' ';

    // copy assignment label
    for (i = 0; i < assignment->label.size; i++)
        buffer[i] = assignment->label.text[i];

    // make buffer null-terminated
    buffer[sizeof(buffer) - 1] = 0;

    // print buffer to lcd
    clcd_cursor_set(lcd, 0, 0);
    clcd_print(lcd, buffer);

    // init buffer with spaces
    for (i = 0; i < sizeof(buffer) - 1; i++)
        buffer[i] = ' ';

    i = 0;
    buffer[i] = '-';

    i = 2;

    // copy item label
    str16_t *item_label = &assignment->list_items[assignment->list_index]->label;
    for (uint8_t j = 0; j < item_label->size && i < (sizeof(buffer)-3); j++, i++)
        buffer[i] = item_label->text[j];

    buffer[15] = '+';

    // make buffer null-terminated
    buffer[sizeof(buffer) - 1] = 0;

    // print buffer to lcd
    clcd_cursor_set(lcd, 1, 0);
    clcd_print(lcd, buffer);
}

static void turn_off_leds(void)
{
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        hw_led_set(i, LED_W, LED_OFF, 0, 0);
    }
}

static void clear_all(void)
{
    turn_off_leds();

    // clear displays
    clcd_clear(0);
    clcd_clear(1);

    // print waiting message for all footswitches
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        waiting_message(i);
}

static void update_leds(cc_assignment_t *assignment)
{
    uint8_t id = assignment->actuator_id;
    if (g_pages_amount > 1)
        id++;

    if ((assignment->mode & CC_MODE_COLOURED) && (assignment->mode & CC_MODE_OPTIONS))
    {
        hw_led_set(id, LED_W, LED_OFF, 0, 0);

        const uint8_t color = ((int)assignment->value % LED_COLOURS_AMOUNT);
        hw_led_set(id, color, LED_ON, 0, 0);

        if (assignment->mode & CC_MODE_GROUP) {
            int led_id = id + ((assignment->mode & CC_MODE_REVERSE)?1:-1);

            hw_led_set(led_id, LED_W, LED_OFF, 0, 0);
            hw_led_set(led_id, color, LED_ON, 0, 0);
        }
    }
    else if ((assignment->mode & CC_MODE_TRIGGER) || (assignment->mode & CC_MODE_OPTIONS))
        hw_led_set(id, LED_G, LED_ON, 0, 0);
    else if (assignment->mode & CC_MODE_TOGGLE)
        hw_led_set(id, LED_R, assignment->value ? LED_ON : LED_OFF,0,0);
    else if (assignment->mode & CC_MODE_TAP_TEMPO)
        hw_led_set(id, LED_G, LED_ON, TAP_TEMPO_TIME_ON, convert_ms(assignment->unit.text, assignment->value, 1) - TAP_TEMPO_TIME_ON);
    else if (assignment->mode & CC_MODE_MOMENTARY)
        hw_led_set(id, LED_R, assignment->value ? LED_ON : LED_OFF,0,0);
}

static void update_lcds(cc_assignment_t *assignment)
{
    if (assignment->mode & CC_MODE_GROUP) {
        lcd_group_widget(assignment);
        return;
    }

    uint8_t id = assignment->actuator_id;
    if (g_pages_amount > 1)
        id++;

    int lcd = (id & 0x02) >> 1;
    int line = id& 0x01;

    char buffer[17];
    uint8_t i;

    // init buffer with spaces
    for (i = 0; i < sizeof(buffer) - 1; i++)
        buffer[i] = ' ';

    // copy assignment label
    for (i = 0; i < assignment->label.size; i++)
        buffer[i] = assignment->label.text[i];

    // copy item label if it's option mode
    if (assignment->mode & (CC_MODE_OPTIONS | CC_MODE_COLOURED))
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
        // force cleaning if still on welcome message
        if (g_welcome_timeout > 0)
        {
            g_welcome_timeout = 0;
            clear_all();

            if (g_pages_amount > 1)
                hw_led_set(0, LED_R, LED_ON, 0, 0);
        }

        cc_assignment_t *assignment = event->data;
        g_current_assignment[assignment->actuator_id] = assignment;

        if (assignment->mode & CC_MODE_TAP_TEMPO)
        {
            uint32_t max;

            // time unit (ms, s)
            if (strcmp(assignment->unit.text, "ms") == 0 || strcmp(assignment->unit.text, "s") == 0)
            {
                max = (uint32_t)(convert_ms(assignment->unit.text, assignment->max, 1) + 0.5);
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
                    max = (uint32_t)(convert_ms(assignment->unit.text, assignment->min, 1) + 0.5);

                //makes sure we enforce a proper timeout
                if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                    max = TAP_TEMPO_DEFAULT_TIMEOUT;
            }

            g_tap_tempo[assignment->actuator_id].time = 0;
            g_tap_tempo[assignment->actuator_id].max = max;
            g_tap_tempo[assignment->actuator_id].state = TT_COUNTING;
        }

        update_leds(assignment);
        update_lcds(assignment);

        serial_flush(g_serial);
    }

    else if (event->id == CC_EV_UNASSIGNMENT)
    {
        int *act_id = event->data;
        int actuator_id = *act_id;
        int hw_actuator_id = actuator_id;

        if (g_pages_amount > 1)
            hw_actuator_id++;

        int lcd = (hw_actuator_id & 0x02) >> 1;
        int line = hw_actuator_id & 0x01;

        // clear lcd line
        clcd_cursor_set(lcd, line, 0);
        clcd_print(lcd, CLEAR_LINE);

        waiting_message(hw_actuator_id);

        // turn off leds
        hw_led_set(hw_actuator_id, LED_W, LED_OFF, 0, 0);

        //properly clear all values
        g_tap_tempo[actuator_id].time = 0;
        g_tap_tempo[actuator_id].max = 0;
        g_tap_tempo[actuator_id].state = TT_INIT;
    }

    else if (event->id == CC_EV_UPDATE)
    {
        cc_assignment_t *assignment = event->data;

        if (assignment->mode != CC_MODE_OPTIONS) {
            update_leds(assignment);
            update_lcds(assignment);
        }
    }

    else if (event->id == CC_CMD_SET_VALUE)
    {
        cc_set_value_t *set_value = event->data;
        cc_assignment_t *assignment = g_current_assignment[set_value->actuator_id];

        if (assignment->mode & CC_MODE_OPTIONS)
            assignment->list_index = set_value->value;

        assignment->value = set_value->value;
        update_leds(assignment);
        update_lcds(assignment);
    }

    else if (event->id == CC_EV_ENUM_UPDATE)
    {
        cc_assignment_t *assignment = event->data;

        if (!(assignment->mode & CC_MODE_GROUP))
            update_lcds(assignment);
    }

    else if (event->id == CC_EV_MASTER_RESETED)
    {
        clear_all();
    }
}

/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

int main(void)
{
    uint8_t alt_boot_modes = hw_init();
    welcome_message(alt_boot_modes);

    // execute self-test if required
    if (alt_boot_modes == BOOT_SELFTEST)
        self_test_run();
    else if (alt_boot_modes == BOOT_SETTINGS)
        settings_screen_run();

    int chain_id = setting_get(CC_CHAIN_ID_ID);
    g_pages_amount = setting_get(PAGE_SETTING_ID);

    for (uint8_t j = 0; j < (FOOTSWITCHES_COUNT); j++)
    {
        g_tap_tempo[j].state = TT_INIT;
    }

    cc_init(response_cb, events_cb);
    cc_device_t *device = cc_device_new("FootEx", "https://github.com/moddevices/cc-fw-footswitch");

    // create actuators
    for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
    {
        if ((g_pages_amount > 1) && (i >= FOOTSWITCHES_COUNT - 1))
            continue;

        char name[16] = {"Foot #"};
        name[6] = '1' + i;
        name[7] = 0;

        cc_actuator_config_t actuator_config;
        actuator_config.type = CC_ACTUATOR_MOMENTARY;
        actuator_config.name = name;
        actuator_config.value = &g_foot_value[(g_pages_amount > 1) ? i+1 : i];
        actuator_config.min = 0.0;
        actuator_config.max = 1.0;
        actuator_config.supported_modes = CC_MODE_TOGGLE | CC_MODE_TRIGGER | CC_MODE_OPTIONS | CC_MODE_TAP_TEMPO | CC_MODE_COLOURED | CC_MODE_MOMENTARY;
        actuator_config.max_assignments = 1;

        cc_actuator_t *actuator = cc_actuator_new(&actuator_config);
        cc_device_actuator_add(device, actuator);
    }

    for (int i = 0; i < CC_MAX_ACTUATORGROUPS; i++)
    {
        if ((g_pages_amount > 1) && (i == 0))
            continue;

        char name[16] = {"Foot x&y"};
        name[5] = '1' + (i*2) - ((g_pages_amount > 1)?1:0);
        name[7] = '2' + (i*2) - ((g_pages_amount > 1)?1:0);
        name[8] = 0;

        cc_actuatorgroup_config_t group;
        group.name = name;
        group.actuator_1 = (i*2) - ((g_pages_amount > 1)?1:0);
        group.actuator_2 = 1 + (i*2) - ((g_pages_amount > 1)?1:0);
        cc_actuatorgroup_t *actuatorgroup = cc_actuatorgroup_new(&group);
        cc_device_actuatorgroup_add(device, actuatorgroup);
    }

    device->actuator_pages = g_pages_amount;
    device->chain_id = chain_id;

    // init serial
    g_serial = serial_init(CC_BAUD_RATE_FALLBACK, serial_recv);

    while (1) {
        if (g_welcome_timeout > 0) {
            if (--g_welcome_timeout == 0) {
                clear_all();
                if (g_pages_amount > 1)
                    hw_led_set(0, LED_R, LED_ON, 0, 0);
            }
        }

        for (int i = 0; i < FOOTSWITCHES_COUNT; i++)
        {
            int button_status = hw_button(i);

            if (button_status == BUTTON_PRESSED)
            {
                //switch pages
                if ((g_pages_amount > 1) && (i == 0))
                {
                    if (g_current_page < g_pages_amount)
                        g_current_page++;
                    else
                        g_current_page = 1;

                    clear_all();

                    hw_led_set(i, LED_W, LED_OFF, 0, 0);
                    //keep colors in sync with Dwarf/DuoX pages
                    switch (g_current_page) {
                        case 1:
                            hw_led_set(i, LED_R, LED_ON, 0, 0);
                        break;

                        case 2:
                            hw_led_set(i, LED_Y, LED_ON, 0, 0);
                        break;

                        case 3:
                            hw_led_set(i, LED_C, LED_ON, 0, 0);
                        break;
                    }

                    print_page_number();

                    //send CC command to request now actuators
                    cc_request_page(g_current_page);

                    //make sure we dont switch pages too fast for this device
                    delay_ms(100);
                }
                else
                {
                    if (g_current_assignment[i]->mode & (CC_MODE_TRIGGER | CC_MODE_OPTIONS) && !(g_current_assignment[i]->mode & CC_MODE_COLOURED))
                    {
                        //update leds
                        hw_led_set(i, LED_G, LED_OFF,0,0);
                        hw_led_set(i, LED_W, LED_ON,0,0);
                    }
                    else if (g_current_assignment[i]->mode & CC_MODE_TAP_TEMPO)
                    {
                        //handle tap tempo
                        handle_tap_tempo(i);
                        continue;
                    }

                    g_foot_value[i] = 1.0;
                }
            }

            else if (button_status == BUTTON_RELEASED)
            {
                if (((g_pages_amount > 1) && (i == 0)) || (g_current_assignment[i]->mode & CC_MODE_TAP_TEMPO))
                {
                    continue;
                }
                else if (g_current_assignment[i]->mode & (CC_MODE_TRIGGER | CC_MODE_OPTIONS) && !(g_current_assignment[i]->mode & CC_MODE_COLOURED))
                {
                    //update leds
                    hw_led_set(i, LED_W, LED_OFF,0,0);
                    hw_led_set(i, LED_G, LED_ON,0,0);
                }

                g_foot_value[i] = 0.0;
            }
        }

        cc_process();
    }

    return 0;
}

#ifdef CCC_ANALYZER
// needed for the static analyzer to link properly
void _start(void) {}
#endif
