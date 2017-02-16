#ifndef CONTROL_CHAIN_H
#define CONTROL_CHAIN_H

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
#include "device.h"
#include "handshake.h"
#include "actuator.h"
#include "update.h"
#include "msg.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

#define CC_PROTOCOL_MAJOR       0
#define CC_PROTOCOL_MINOR       0


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

// maximum number of devices that can created
#define CC_MAX_DEVICES      1
// maximum number of actuators that can created per device
#define CC_MAX_ACTUATORS    4
// maximum number of assignments that can created per actuator
#define CC_MAX_ASSIGNMENTS  1


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef struct cc_data_t {
    uint8_t *data;
    uint32_t size;
} cc_data_t;

typedef struct cc_event_t {
    int id;
    void *data;
} cc_event_t;

enum {CC_EV_HANDSHAKE_FAILED, CC_EV_ASSIGNMENT, CC_EV_UNASSIGNMENT, CC_EV_UPDATE,
      CC_EV_DEVICE_DISABLED, CC_EV_MASTER_RESETED};


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

void cc_init(void (*response_cb)(void *arg), void (*events_cb)(void *arg));
void cc_process(void);
void cc_parse(const cc_data_t *received);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif
