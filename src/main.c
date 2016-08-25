#include "hardware.h"
#include "control_chain.h"
#include "actuator.h"

int main(void)
{
    hw_init();
    cc_init();

    static volatile float foot2;
    cc_actuator_new(&foot2);

    while (1)
    {
        if (hw_button(1))
        {
            hw_led(3, LED_R, LED_ON);
            foot2 = 1.0;
        }
        else
        {
            hw_led(3, LED_R, LED_OFF);
            foot2 = 0.0;
        }

        cc_process();
    }

    return 0;
}
