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
static node_t *g_nodes_cache[MAX_ASSIGNMENTS];
static cc_assignment_t g_assignments_cache[MAX_ASSIGNMENTS];


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static void cc_assignment_init(void)
{
    g_assignments = node_create(0);

    // create a cache of unconnected nodes to be used when an assignment add is done
    // using the cache instead of create/destroy a node each time an assignment is added/removed
    // prevents memory fragmentation
    for (int i = 0; i < MAX_ASSIGNMENTS; i++)
    {
        g_nodes_cache[i] = node_create(0);
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void cc_assignment_add(cc_assignment_t *assignment)
{
    if (!g_assignments)
        cc_assignment_init();

    // search for an unused node
    for (int i = 0; i < MAX_ASSIGNMENTS; i++)
    {
        node_t *node = g_nodes_cache[i];

        // if data is null, node is free
        if (!node->data)
        {
            cc_assignment_t *cache = &g_assignments_cache[i];
            node->data = cache;
            cache->id = assignment->id;
            cache->actuator_id = assignment->actuator_id;
            cache->value = assignment->value;
            cache->min = assignment->min;
            cache->max = assignment->max;
            cache->mode = assignment->mode;
            node_append(g_assignments, node);
            cc_actuator_map(cache);
            break;
        }
    }
}

void cc_assignment_remove(int assignment_id)
{
    cc_assignments_t *assignments;
    for (assignments = cc_assignments(); assignments; assignments = assignments->next)
    {
        cc_assignment_t *assignment = assignments->data;
        if (assignment_id == assignment->id)
        {
            cc_actuator_unmap(assignment);
            assignments->data = 0;
            node_cut(assignments);
            break;
        }
    }
}

cc_assignments_t *cc_assignments(void)
{
    if (g_assignments)
        return g_assignments->first;

    return 0;
}
