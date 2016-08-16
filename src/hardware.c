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


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/


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

    // led 1
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 21);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 8);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 31);
    Chip_GPIO_SetPinState(LPC_GPIO, 1, 21, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 8, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 1, 31, 1);

    // led 2
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 23);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 21);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 1, 23, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 21, 1);

    // led 3
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 13);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 12);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 14);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 13, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 12, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 14, 1);

    // led 4
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 29);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);
    Chip_GPIO_SetPinState(LPC_GPIO, 1, 29, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, 1);
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, 1);

#if 0
    // switches
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 7);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 28);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 17);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 1, 15);

    // lcd backlight
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 18);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 24);

#endif
}
