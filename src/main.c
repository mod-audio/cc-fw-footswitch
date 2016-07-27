#include "chip.h"

const uint32_t OscRateIn = 12000000;
const uint32_t ExtRateIn = 0;

int main(void)
{
    Chip_SystemInit();

    Chip_GPIO_Init(LPC_GPIO);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 21);
    Chip_GPIO_SetPinState(LPC_GPIO, 1, 21, 0);

    while (1);

    return 0;
}
