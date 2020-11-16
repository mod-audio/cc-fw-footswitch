/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdlib.h>
#include "hardware.h"
#include "chip.h"
#include "gpio.h"
#include "delay.h"
#include "clcd.h"

/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define N_BUTTONS       (sizeof(g_buttons_gpio)/sizeof(gpio_t))
#define N_LEDS          (sizeof(g_leds_gpio)/sizeof(gpio_t))
#define N_BACKLIGHTS    (sizeof(g_backlights_gpio)/sizeof(gpio_t))

/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/

const uint32_t OscRateIn = 12000000;
const uint32_t ExtRateIn = 0;

static const gpio_t g_buttons_gpio[] = {BUTTONS_PINS};
static const gpio_t g_leds_gpio[] = {LEDS_PINS};
static const gpio_t g_backlights_gpio[] = {BACKLIGHTS_PINS};

/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

typedef struct button_t {
    int state, event;
    unsigned int count;
} button_t;

typedef struct blinking_led_t {
    uint8_t state;
    int8_t color[N_LEDS];
    uint16_t on_time, off_time, time;
} blinking_led_t;

/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static button_t g_buttons[N_BUTTONS];
static uint32_t g_counter, g_self_test;
static blinking_led_t g_blinking_led[N_LEDS];

/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

// buttons + led proces
void SysTick_Handler(void)
{
    g_counter++;

    for (int i = 0; i < N_BUTTONS; i++)
    {
        const gpio_t *gpio = &g_buttons_gpio[i];
        button_t *button = &g_buttons[i];

        int value = (Chip_GPIO_GetPinState(LPC_GPIO, gpio->port, gpio->pin) == 0 ? 1 : 0);
        if (value)
        {
            if (button->state == BUTTON_RELEASED)
            {
                button->count++;
                if (button->count >= BUTTON_DEBOUNCE)
                {
                    button->count = 0;
                    button->state = BUTTON_PRESSED;
                    button->event = BUTTON_PRESSED;
                }
            }
        }
        else
        {
            if (button->state == BUTTON_PRESSED)
            {
                button->count++;
                if (button->count >= BUTTON_DEBOUNCE)
                {
                    button->count = 0;
                    button->state = BUTTON_RELEASED;
                    button->event = BUTTON_RELEASED;
                }
            }
        }
    }

    for (int i = 0; i < N_LEDS; i++)
    {
        blinking_led_t *led = &g_blinking_led[i];

        if ((led->on_time != 0) && (led->off_time != 0))
        {
            //blink get current time
            if (led->time == 0)
                led->time = hw_uptime();

            //on or off
            if ((hw_uptime() - led->time) > led->on_time)
            {
                //turn led off
                if (led->state == LED_ON)
                {
                    led->state = LED_OFF;
                    for (uint8_t j=0; j < 3; j++)
                    {
                        if (led->color[j] != -1)
                            hw_led(i, led->color[j], led->state);
                    }
                }
            }

            if ((hw_uptime() - led->time) > (led->off_time + led->on_time))
            {
                //turn led off
                if (led->state == LED_OFF)
                {
                    led->state = LED_ON;
                    for (uint8_t j=0; j < 3; j++)
                    {
                        if (led->color[j] != -1)
                            hw_led(i, led->color[j], led->state);
                    }
                }

                led->time = 0;
            }
        } 
    }
}

// read unique id via IAP
static void read_uid(uint32_t *ptr)
{
    unsigned int param_table[5];
    unsigned int result_table[5];

    param_table[0] = 58; // IAP command
    iap_entry(param_table, result_table);

    if(result_table[0] == 0)
    {
        ptr[0] = result_table[1];
        ptr[1] = result_table[2];
        ptr[2] = result_table[3];
        ptr[3] = result_table[4];
    }
}

static unsigned int generate_seed(void)
{
    uint32_t uid[4] = {0, 0, 0, 0};
    read_uid(uid);

    uint32_t seed = 0;
    for (int i = 0; i < 4; i++)
    {
        seed ^= uid[i];
    }

    return seed;
}

/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void hw_init(void)
{
    Chip_SystemInit();
    SystemCoreClockUpdate();

    Chip_GPIO_Init(LPC_GPIO);

    // configure JTAG pins to work as I/Os
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 11, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC1);

    // delay
    delay_init();

    // leds
    for (int i = 0; i < N_LEDS; i++)
    {
        const gpio_t *gpio = &g_leds_gpio[i];
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, gpio->port, gpio->pin);
        Chip_GPIO_SetPinState(LPC_GPIO, gpio->port, gpio->pin, 1);
    }

    // buttons
    for (int i = 0; i < N_BUTTONS; i++)
    {
        const gpio_t *gpio = &g_buttons_gpio[i];
        Chip_GPIO_SetPinDIRInput(LPC_GPIO, gpio->port, gpio->pin);
        g_buttons[i].event = -1;
    }

    // LCD
    static const clcd_gpio_t lcd1_gpio = LCD1_PINS;
    static const clcd_gpio_t lcd2_gpio = LCD2_PINS;
    clcd_init(CLCD_4BIT | CLCD_2LINE, &lcd1_gpio);
    clcd_init(CLCD_4BIT | CLCD_2LINE, &lcd2_gpio);

    // backlights
    for (int i = 0; i < N_BACKLIGHTS; i++)
    {
        const gpio_t *gpio = &g_backlights_gpio[i];
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, gpio->port, gpio->pin);
        Chip_GPIO_SetPinState(LPC_GPIO, gpio->port, gpio->pin, 1);
    }

    SysTick_Config(SystemCoreClock / 1000);

    // init random generator
    srand(generate_seed());

    // wait some time to read the buttons
    delay_ms(100);

    // check if should start in self-test mode
    int foot3 = hw_button(2), foot4 = hw_button(3);
    if (foot3 == BUTTON_PRESSED && foot4 == BUTTON_PRESSED)
        g_self_test = 1;
}

int hw_button(int button)
{
    int event = g_buttons[button].event;
    g_buttons[button].event = -1;

    return event;
}

void hw_led(int led, int color, int value)
{
    const gpio_t *l = &g_leds_gpio[(led * 3) + color];

    if (value == LED_OFF)
        Chip_GPIO_SetPinState(LPC_GPIO, l->port, l->pin, 1);
    else if (value == LED_ON)
        Chip_GPIO_SetPinState(LPC_GPIO, l->port, l->pin, 0);
    else if (value == LED_TOGGLE)
        Chip_GPIO_SetPinToggle(LPC_GPIO, l->port, l->pin);
}

void hw_led_set(int led, int color, int value, int on_time_ms, int off_time_ms)
{
    int colors[3] = {0, 0, 0};

    if (color == LED_W)
    {
        colors[0] = LED_R;
        colors[1] = LED_G;   
        colors[2] = LED_B;
    }
    else if (color == LED_Y)
    {
        colors[0] = LED_R;
        colors[1] = LED_G;
        colors[2] = -1;   
    }
    else if(color == LED_C)
    {
        colors[0] = LED_B;
        colors[1] = LED_G;
        colors[2] = -1;   
    }
    else if (color == LED_M)
    {
        colors[0] = LED_R;
        colors[1] = LED_B;
        colors[2] = -1;   
    }
    else if (color == LED_R)
    {
        colors[0] = LED_R;
        colors[1] = -1;
        colors[2] = -1;   
    }
    else if (color == LED_G)
    {
        colors[0] = -1;
        colors[1] = -1;
        colors[2] = LED_G;   
    }
    else if (color == LED_B)
    {
        colors[0] = -1;
        colors[1] = LED_B;
        colors[2] = -1;   
    }

    for (uint8_t j=0; j < 3; j++)
    {
        if (colors[j] != -1)
            hw_led(led, colors[j], value);
    }

    //set tap tempo constants
    g_blinking_led[led].on_time = on_time_ms;
    g_blinking_led[led].off_time = off_time_ms;
    g_blinking_led[led].time = 0;
    g_blinking_led[led].state = value;

    for (uint8_t q = 0; q < 3; q++)
    {
        g_blinking_led[led].color[q] = colors[q];
    }
}


inline uint32_t hw_uptime(void)
{
    return g_counter;
}

inline int hw_self_test(void)
{
    return g_self_test;
}
