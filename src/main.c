#include "hardware.h"
#include "control_chain.h"
#include "actuator.h"

int main(void)
{
    hw_init();
    cc_init();

    static volatile float foots[4];

    for (int i = 0; i < 4; i++)
    {
        cc_actuator_new(&foots[i]);
    }

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            int button_status = hw_button(i);
            if (button_status == BUTTON_PRESSED)
            {
                hw_led(i, LED_R, LED_ON);
                foots[i] = 1.0;
            }
            else if (button_status == BUTTON_RELEASED)
            {
                hw_led(i, LED_R, LED_OFF);
                foots[i] = 0.0;
            }
        }

        cc_process();
    }

    return 0;
}
