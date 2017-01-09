/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdlib.h>
#include "device.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define CC_MAX_DEVICES  1


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

static cc_device_t g_devices[CC_MAX_DEVICES];
static unsigned int g_devices_count;


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

cc_device_t *cc_device_new(const char *name, const char *uri)
{
    if (g_devices_count >= CC_MAX_DEVICES)
        return 0;

    cc_device_t *device = &g_devices[g_devices_count];

    // create device URI and label
    device->uri = string_create(uri);
    device->label = string_create(name);

    // create a list of actuators
    device->actuators = malloc(sizeof(cc_actuator_t *) * CC_MAX_ACTUATORS);
    device->actuators_count = 0;

    g_devices_count++;

    return device;
}

void cc_device_actuator_add(cc_device_t *device, cc_actuator_t *actuator)
{
    device->actuators[device->actuators_count] = actuator;
    device->actuators_count++;
}

cc_device_t *cc_device_get(void)
{
    return &g_devices[0];
}
