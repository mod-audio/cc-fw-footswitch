/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <string.h>
#include "control_chain.h"
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
static void (*g_assignments_cb)(void *arg);


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
            memcpy(&cache->label, &assignment->label, sizeof(str16_t));
            lili_push(g_assignments, cache);
            cc_actuator_map(cache);

            // callback
            if (g_assignments_cb)
            {
                cc_event_t event;
                event.id = CC_CMD_ASSIGNMENT;
                event.data = cache;
                g_assignments_cb(&event);
            }

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

            // callback
            if (g_assignments_cb)
            {
                cc_event_t event;
                event.id = CC_CMD_UNASSIGNMENT;
                event.data = assignment;
                g_assignments_cb(&event);
            }

            assignment->id = -1;
            break;
        }

        index++;
    }
}

void cc_assignments_callback(void (*assignments_cb)(void *arg))
{
    g_assignments_cb = assignments_cb;
}

cc_assignments_t *cc_assignments(void)
{
    return g_assignments;
}
