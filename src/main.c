#include "hardware.h"
#include "control_chain.h"

int main(void)
{
    hw_init();
    cc_init();

    while (1);
//        cc_process();

    return 0;
}
