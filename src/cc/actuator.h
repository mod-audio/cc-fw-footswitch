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

#define CC_MAX_ACTUATORS   4


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

enum {CC_ACTUATOR_CONTINUOUS, CC_ACTUATOR_DISCRETE, CC_ACTUATOR_SWITCH, CC_ACTUATOR_MOMENTARY};

typedef struct cc_actuator_t {
    int id, type;
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
cc_actuator_t *cc_actuator_new(int type, float *var, float min, float max);
// map assignment to actuator
void cc_actuator_map(cc_assignment_t *assignment);
// unmap assignment from actuator
void cc_actuator_unmap(cc_assignment_t *assignment);
// return a list with all created actuators
cc_actuator_t **cc_actuators(unsigned int *n_actuators);
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
