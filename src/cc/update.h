#ifndef UPDATE_H
#define UPDATE_H


/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "node.h"


/*
************************************************************************************************************************
*       MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       CONFIGURATION
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       DATA TYPES
************************************************************************************************************************
*/

typedef node_t cc_updates_t;

typedef struct cc_update_t {
    uint8_t assignment_id;
    float value;
} cc_update_t;


/*
************************************************************************************************************************
*       FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void cc_update_append(cc_update_t *update);
void cc_update_clean(void);
cc_updates_t *cc_updates(void);


/*
************************************************************************************************************************
*   CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*   END HEADER
************************************************************************************************************************
*/

#endif