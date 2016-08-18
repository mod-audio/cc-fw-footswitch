/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include "update.h"


/*
************************************************************************************************************************
*       INTERNAL MACROS
************************************************************************************************************************
*/

#define MAX_UPDATES     10


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

static cc_updates_t *g_updates = 0;
static node_t *g_nodes_cache[MAX_UPDATES];
static cc_update_t g_updates_cache[MAX_UPDATES];


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/

static void cc_update_init(void)
{
    g_updates = node_create(0);

    // create a cache of unconnected nodes to be used when an update add is done
    // using the cache instead of create/destroy a node each time an update is added/removed
    // prevents memory fragmentation
    for (int i = 0; i < MAX_UPDATES; i++)
    {
        g_nodes_cache[i] = node_create(0);
    }
}


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void cc_update_append(cc_update_t *update)
{
    if (!g_updates)
        cc_update_init();

    // search for an unused node
    for (int i = 0; i < MAX_UPDATES; i++)
    {
        node_t *node = g_nodes_cache[i];

        // if data is null, node is free
        if (!node->data)
        {
            node->data = &g_updates_cache[i];
            cc_update_t *cache = (cc_update_t *) node->data;
            cache->assignment_id = update->assignment_id;
            cache->value = update->value;
            node_append(g_updates, node);
            break;
        }
    }
}

void cc_update_clean(void)
{
    for (cc_updates_t *updates = cc_updates(); updates; updates = updates->next)
    {
        node_cut(updates);
        updates->data = 0;
    }
}

cc_updates_t *cc_updates(void)
{
    if (g_updates)
        return g_updates->first;

    return 0;
}
