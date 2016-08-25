/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "chip.h"
#include "hardware.h"


/*
************************************************************************************************************************
*       INTERNAL MACROS
************************************************************************************************************************
*/

#define N_BUTTONS       (sizeof(g_buttons_gpio)/sizeof(gpio_t))
#define N_LEDS          (sizeof(g_leds_gpio)/sizeof(gpio_t))
#define N_BACKLIGHTS    (sizeof(g_backlights_gpio)/sizeof(gpio_t))


/*
************************************************************************************************************************
*       INTERNAL CONSTANTS
************************************************************************************************************************
*/

const uint32_t OscRateIn = 12000000;
const uint32_t ExtRateIn = 0;


/*
************************************************************************************************************************
*       INTERNAL DATA TYPES
************************************************************************************************************************
*/

typedef struct gpio_t
{
    int port, pin;
} gpio_t;

typedef struct button_t
{
    int state, event;
    unsigned int count;
} button_t;


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static const gpio_t g_buttons_gpio[] = {BUTTONS_PINS};
static const gpio_t g_leds_gpio[] = {LEDS_PINS};
static const gpio_t g_backlights_gpio[] = {BACKLIGHTS_PINS};
static button_t g_buttons[N_BUTTONS];


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/
// buttons process
void SysTick_Handler(void)
{
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


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
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

    // backlights
    for (int i = 0; i < N_BACKLIGHTS; i++)
    {
        const gpio_t *gpio = &g_backlights_gpio[i];
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, gpio->port, gpio->pin);
        Chip_GPIO_SetPinState(LPC_GPIO, gpio->port, gpio->pin, 0);
    }


    SysTick_Config(SystemCoreClock / 1000);
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
