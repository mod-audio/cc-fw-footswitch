#ifndef CC_ACTUATOR_H
#define CC_ACTUATOR_H

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
#include "assignment.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/


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

typedef lili_t cc_actuators_t;

typedef struct cc_actuator_t {
    int id;
    volatile float *value;
    float min, max;
    cc_assignment_t *assignment;
    int lock;
} cc_actuator_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

// create a new actuator object
cc_actuator_t *cc_actuator_new(volatile float *var);
// map assignment to actuator
void cc_actuator_map(cc_assignment_t *assignment);
// unmap assignment from actuator
void cc_actuator_unmap(cc_assignment_t *assignment);
// return a list with all created actuators
cc_actuators_t *cc_actuators(void);
// process the assignments of all created actuators
void cc_actuators_process(void);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif
