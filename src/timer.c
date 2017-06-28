/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "chip.h"
#include "timer.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

void (*g_callback)(void);


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void timer_init(void (*callback)(void))
{
    // enable timer 1 clock
    Chip_TIMER_Init(LPC_TIMER32_0);

    // timer setup for match and interrupt
    Chip_TIMER_Reset(LPC_TIMER32_0);
    Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);
    Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_0, 1);

    // enable timer interrupt
    NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
    NVIC_EnableIRQ(TIMER_32_0_IRQn);

    g_callback = callback;
}

void timer_set(uint32_t time_us)
{
    Chip_TIMER_Disable(LPC_TIMER32_0);
    Chip_TIMER_Reset(LPC_TIMER32_0);

    // timer rate is system clock rate
    uint32_t timer_freq = Chip_Clock_GetSystemClockRate();
    Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, timer_freq / (1000000 / time_us));

    Chip_TIMER_Enable(LPC_TIMER32_0);
}

void TIMER32_0_IRQHandler(void)
{
    if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1))
    {
        Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
        Chip_TIMER_Disable(LPC_TIMER32_0);

        if (g_callback)
            g_callback();
    }
}
