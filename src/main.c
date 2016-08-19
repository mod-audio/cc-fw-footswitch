#include "hardware.h"
#include "chip.h"
#include "control_chain.h"
#include "actuator.h"

int main(void)
{
    hw_init();
    cc_init();

    static volatile float foot2 = 3.141593;
    cc_actuator_new(&foot2);

    while (1)
    {
        if (Chip_GPIO_GetPinState(LPC_GPIO, 1, 28) == 0)
        {
            Chip_GPIO_SetPinState(LPC_GPIO, 1, 29, 0);
            foot2 = 1.0;
        }
        else
        {
            Chip_GPIO_SetPinState(LPC_GPIO, 1, 29, 1);
            foot2 = 0.0;
        }

        //cc_process();
    }

    return 0;
}
