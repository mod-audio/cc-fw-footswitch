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

#define N_BUTTONS       (sizeof(g_buttons)/sizeof(pin_t))
#define N_LEDS          (sizeof(g_leds)/sizeof(pin_t))
#define N_BACKLIGHTS    (sizeof(g_backlights)/sizeof(pin_t))


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

typedef struct pin_t
{
    int port, pin;
} pin_t;


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static const pin_t g_buttons[] = {BUTTONS_PINS};
static const pin_t g_leds[] = {LEDS_PINS};
static const pin_t g_backlights[] = {BACKLIGHTS_PINS};


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void hw_init(void)
{
    Chip_SystemInit();

    Chip_GPIO_Init(LPC_GPIO);

    // configure JTAG pins to work as I/Os
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 11, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, FUNC1);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC1);

    // leds
    for (int i = 0; i < N_LEDS; i++)
    {
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, g_leds[i].port, g_leds[i].pin);
        Chip_GPIO_SetPinState(LPC_GPIO, g_leds[i].port, g_leds[i].pin, 1);
    }

    // buttons
    for (int i = 0; i < N_BUTTONS; i++)
    {
        Chip_GPIO_SetPinDIRInput(LPC_GPIO, g_buttons[i].port, g_buttons[i].pin);
    }

    // backlights
    for (int i = 0; i < N_BACKLIGHTS; i++)
    {
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, g_backlights[i].port, g_backlights[i].pin);
        Chip_GPIO_SetPinState(LPC_GPIO, g_backlights[i].port, g_backlights[i].pin, 0);
    }
}

int hw_button(int button)
{
    const pin_t *b = &g_buttons[button];
    return (Chip_GPIO_GetPinState(LPC_GPIO, b->port, b->pin) == 0 ? 1 : 0);
}

void hw_led(int led, int color, int value)
{
    const pin_t *l = &g_leds[(led * 3) + color];

    if (value == LED_OFF)
        Chip_GPIO_SetPinState(LPC_GPIO, l->port, l->pin, 1);
    else if (value == LED_ON)
        Chip_GPIO_SetPinState(LPC_GPIO, l->port, l->pin, 0);
    else if (value == LED_TOGGLE)
        Chip_GPIO_SetPinToggle(LPC_GPIO, l->port, l->pin);
}
