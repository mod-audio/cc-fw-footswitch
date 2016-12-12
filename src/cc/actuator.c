/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "actuator.h"
#include "update.h"
#include <math.h>


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

static cc_actuators_t *g_actuators = 0;
static cc_actuator_t g_actuators_cache[CC_MAX_ACTUATORS];
static unsigned int g_actuators_count;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static int update_assignment_value(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    if (assignment->mode & (CC_MODE_TOGGLE | CC_MODE_TRIGGER))
    {
        float actuator_value = *(actuator->value);

        if (actuator_value > 0.0)
        {
            if (actuator->lock == 0)
            {
                actuator->lock = 1;

                if (assignment->mode & CC_MODE_TOGGLE)
                    assignment->value = 1.0 - assignment->value;
                else
                    assignment->value = 1.0;

                return 1;
            }
        }
        else
        {
            actuator->lock = 0;
        }
    }

#if 0
    float a, b;
    a = (assignment->max - assignment->min) / (actuator->max - actuator->min);
    b = assignment->min - a*actuator->min;

    float value, actuator_value = *(actuator->value);
    value = a*actuator_value + b;

    if (abs(assignment->value - value) >= 0.0001)
    {
        assignment->value = value;
        return 1;
    }
#endif

    return 0;
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

cc_actuator_t *cc_actuator_new(volatile float *var)
{
    if (!g_actuators)
        g_actuators = lili_create();

    if (g_actuators_count >= CC_MAX_ACTUATORS)
        return 0;

    cc_actuator_t *actuator = &g_actuators_cache[g_actuators_count];

    // initialize actuator data struct
    actuator->id = g_actuators_count;

    // FIXME: for initial tests
    actuator->min = 0.0;
    actuator->max = 1.0;
    actuator->value = var;

    // append new actuator to actuators list
    lili_push(g_actuators, actuator);
    g_actuators_count++;

    return actuator;
}

void cc_actuator_map(cc_assignment_t *assignment)
{
    LILI_FOREACH(g_actuators, node)
    {
        cc_actuator_t *actuator = node->data;
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = assignment;
            break;
        }
    }
}

void cc_actuator_unmap(cc_assignment_t *assignment)
{
    LILI_FOREACH(g_actuators, node)
    {
        cc_actuator_t *actuator = node->data;
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = 0;
            break;
        }
    }
}

cc_actuators_t *cc_actuators(void)
{
    return g_actuators;
}

void cc_actuators_process(void)
{
    LILI_FOREACH(g_actuators, node)
    {
        cc_actuator_t *actuator = node->data;
        cc_assignment_t *assignment = actuator->assignment;

        if (!assignment)
            continue;

        // update assignment value according current actuator value
        int updated = update_assignment_value(actuator, assignment);
        if (updated)
        {
            // append update to be sent
            cc_update_t update;
            update.assignment_id = assignment->id;
            update.value = assignment->value;
            cc_update_push(&update);
        }
    }
}
