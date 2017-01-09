#ifndef CC_ASSIGNMENT_H
#define CC_ASSIGNMENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "lili.h"
#include "utils.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

#define CC_MODE_TOGGLE  0x01
#define CC_MODE_TRIGGER 0x02


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef lili_t cc_assignments_t;

typedef struct cc_assignment_t {
    int id, actuator_id;
    str16_t label;
    float value, min, max, def;
    uint32_t mode;
} cc_assignment_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

// add an assignment to the assignments list
void cc_assignment_add(cc_assignment_t *assignment);
// remove an assignment from the assignments list
void cc_assignment_remove(int assignment_id);
// set callback to assignments events
void cc_assignments_callback(void (*assignments_cb)(void *arg));
// return the assignments list
cc_assignments_t *cc_assignments(void);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif
