/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "control_chain.h"
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

static cc_actuator_t g_actuators[CC_MAX_ACTUATORS];
static unsigned int g_actuators_count;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static int momentary_process(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    float actuator_value = *(actuator->value);
    if (actuator_value > 0.0)
    {
        if (actuator->lock == 0)
        {
            actuator->lock = 1;

            if (assignment->mode & CC_MODE_TOGGLE)
            {
                assignment->value = 1.0 - assignment->value;
            }
            else if (assignment->mode & CC_MODE_TRIGGER)
            {
                assignment->value = 1.0;
            }

            return 1;
        }
    }
    else
    {
        actuator->lock = 0;
    }

    return 0;
}

static int update_assignment_value(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
    switch (actuator->type)
    {
        case CC_ACTUATOR_MOMENTARY:
            return momentary_process(actuator, assignment);
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

cc_actuator_t *cc_actuator_new(cc_actuator_config_t *config)
{
    if (g_actuators_count >= CC_MAX_ACTUATORS)
        return 0;

    cc_actuator_t *actuator = &g_actuators[g_actuators_count];

    // initialize actuator data struct
    actuator->id = g_actuators_count;
    actuator->type = config->type;
    actuator->value = config->value;
    actuator->min = config->min;
    actuator->max = config->max;
    actuator->supported_modes = config->supported_modes;
    actuator->max_assignments = config->max_assignments;
    str16_create(config->name, &actuator->name);

    g_actuators_count++;

    return actuator;
}

void cc_actuator_map(cc_assignment_t *assignment)
{
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = assignment;
            break;
        }
    }
}

void cc_actuator_unmap(cc_assignment_t *assignment)
{
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = 0;
            break;
        }
    }
}

void cc_actuators_process(void (*events_cb)(void *arg))
{
    for (int i = 0; i < g_actuators_count; i++)
    {
        cc_actuator_t *actuator = &g_actuators[i];
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

            if (events_cb)
            {
                cc_event_t event;
                event.id = CC_EV_UPDATE;
                event.data = assignment;
                events_cb(&event);
            }
        }
    }
}
