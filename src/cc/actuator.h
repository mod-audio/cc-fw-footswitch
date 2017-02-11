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

typedef struct cc_actuator_config_t {
    int type;
    const char *name;
    volatile float *value;
    float min, max;
    uint32_t supported_modes;
    int max_assignments;
} cc_actuator_config_t;

typedef struct cc_actuator_t {
    int id, type, lock;
    str16_t name;
    volatile float *value;
    float min, max;
    uint32_t supported_modes;
    int max_assignments;
    cc_assignment_t *assignment;
} cc_actuator_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

// create a new actuator object
cc_actuator_t *cc_actuator_new(cc_actuator_config_t *config);
// map assignment to actuator
void cc_actuator_map(cc_assignment_t *assignment);
// unmap assignment from actuator
void cc_actuator_unmap(cc_assignment_t *assignment);
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
