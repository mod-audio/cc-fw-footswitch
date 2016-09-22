#include "hardware.h"
#include "control_chain.h"
#include "actuator.h"
#include "update.h"

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
                foots[i] = 1.0;
            }
            else if (button_status == BUTTON_RELEASED)
            {
                foots[i] = 0.0;
            }
        }

        cc_process();

        for (int i = 0; i < 4; i++)
            hw_led(i, LED_R, LED_OFF);

        cc_assignments_t *assignments = cc_assignments();
        if (assignments)
        {
            LILI_FOREACH(assignments, node)
            {
                cc_assignment_t *assignment = node->data;
                if (assignment->mode == CC_MODE_TOGGLE)
                    hw_led(assignment->actuator_id, LED_R, assignment->value ? LED_ON : LED_OFF);
            }
        }
    }

    return 0;
}
