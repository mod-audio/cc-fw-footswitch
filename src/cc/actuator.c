/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include "actuator.h"
#include "update.h"


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

static cc_actuators_t *g_actuators = 0;
static unsigned int g_actuators_count;


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/

static void assignment_update(cc_actuator_t *actuator, cc_assignment_t *assignment)
{
}


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
*/

cc_actuator_t *cc_actuator_new(void)
{
    if (!g_actuators)
        g_actuators = node_create(0);

    cc_actuator_t *actuator = (cc_actuator_t *) malloc(sizeof (cc_actuator_t));

    // initialize actuator data struct
    actuator->id = g_actuators_count++;
    actuator->assignment = 0;

    // append new actuator to actuators list
    node_child(g_actuators, actuator);

    return actuator;
}

void cc_actuator_map(cc_assignment_t *assignment)
{
    cc_actuators_t *actuators;
    for (actuators = cc_actuators(); actuators; actuators = actuators->next)
    {
        cc_actuator_t *actuator = actuators->data;
        if (actuator->id == assignment->actuator_id)
        {
            actuator->assignment = assignment;
            break;
        }
    }
}

cc_actuators_t *cc_actuators(void)
{
    if (g_actuators)
        return g_actuators->first;

    return 0;
}

void cc_actuators_process(void)
{
    cc_actuators_t *actuators;
    for (actuators = cc_actuators(); actuators; actuators = actuators->next)
    {
        cc_actuator_t *actuator = actuators->data;
        cc_assignment_t *assignment = actuator->assignment;

        if (assignment)
        {
            // update assignment value according current actuator value
            assignment_update(actuator, assignment);

            // append update to be sent
            cc_update_t update;
            update.assignment_id = assignment->id;
            update.value = assignment->value;
            cc_update_append(&update);
        }
    }
}
