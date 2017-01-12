/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "update.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define MAX_UPDATES     10


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

static cc_updates_t *g_updates = 0;
static cc_update_t g_updates_cache[MAX_UPDATES];


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

void cc_update_push(const cc_update_t *update)
{
    // initialize update list and cache
    if (!g_updates)
    {
        g_updates = lili_create();

        for (int i = 0; i < MAX_UPDATES; i++)
            g_updates_cache[i].assignment_id = -1;
    }

    // search for unused update
    for (int i = 0; i < MAX_UPDATES; i++)
    {
        cc_update_t *cache = &g_updates_cache[i];
        if (cache->assignment_id == -1)
        {
            cache->assignment_id = update->assignment_id;
            cache->value = update->value;
            lili_push(g_updates, cache);
            break;
        }
    }
}

int cc_update_pop(cc_update_t *update)
{
    cc_update_t *cache = lili_pop_front(g_updates);

    if (cache)
    {
        if (update)
        {
            update->assignment_id = cache->assignment_id;
            update->value = cache->value;
        }

        // make cache position available again
        cache->assignment_id = -1;

        return 1;
    }

    return 0;
}

cc_updates_t *cc_updates(void)
{
    return g_updates;
}

void cc_updates_clear(void)
{
    while (cc_update_pop(0));
}
