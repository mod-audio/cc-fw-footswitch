/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "assignment.h"
#include "device.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define MAX_ASSIGNMENTS     4


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

static cc_assignments_t *g_assignments = 0;
static cc_assignment_t g_assignments_cache[MAX_ASSIGNMENTS];


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

void cc_assignment_add(cc_assignment_t *assignment)
{
    // initialize assignments list and cache
    if (!g_assignments)
    {
        g_assignments = lili_create();

        for (int i = 0; i < MAX_ASSIGNMENTS; i++)
            g_assignments_cache[i].id = -1;
    }

    // search for unused assignments
    for (int i = 0; i < MAX_ASSIGNMENTS; i++)
    {
        cc_assignment_t *cache = &g_assignments_cache[i];
        if (cache->id == -1)
        {
            cache->id = assignment->id;
            cache->actuator_id = assignment->actuator_id;
            cache->value = assignment->value;
            cache->min = assignment->min;
            cache->max = assignment->max;
            cache->mode = assignment->mode;
            lili_push(g_assignments, cache);
            cc_actuator_map(cache);
            break;
        }
    }
}

void cc_assignment_remove(int assignment_id)
{
    int index = 0;
    LILI_FOREACH(g_assignments, node)
    {
        cc_assignment_t *assignment = node->data;
        if (assignment_id == assignment->id)
        {
            cc_actuator_unmap(assignment);
            lili_pop_from(g_assignments, index);
            assignment->id = -1;
            break;
        }

        index++;
    }
}

cc_assignments_t *cc_assignments(void)
{
    return g_assignments;
}
