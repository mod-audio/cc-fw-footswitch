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


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

typedef struct button_t {
    int state, event;
    unsigned int count;
} button_t;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static const gpio_t g_buttons_gpio[] = {BUTTONS_PINS};
static const gpio_t g_leds_gpio[] = {LEDS_PINS};
static const gpio_t g_backlights_gpio[] = {BACKLIGHTS_PINS};
static button_t g_buttons[N_BUTTONS];
static uint32_t g_counter;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

// buttons process
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

inline uint32_t hw_uptime(void)
{
    return g_counter;
}
