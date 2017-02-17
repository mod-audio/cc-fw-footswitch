/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "msg.h"
#include "handshake.h"
#include "device.h"
#include "update.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define MAX_INSTANCES       2
#define DATA_BUFFER_SIZE    128


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

struct cc_msg_manager_t {
    int count;
    cc_msg_t instances[MAX_INSTANCES];
    uint8_t buffers[MAX_INSTANCES][DATA_BUFFER_SIZE];
} g_msg_man;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/


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

cc_msg_t *cc_msg_new(void)
{
    cc_msg_t *msg;

    int i = g_msg_man.count++;
    msg = &g_msg_man.instances[i];
    msg->header = g_msg_man.buffers[i];
    msg->data = &msg->header[CC_MSG_HEADER_SIZE];

    return msg;
}

int cc_msg_parser(const cc_msg_t *msg, void *data_struct)
{
    uint8_t *pdata = msg->data;

    if (msg->command == CC_CMD_HANDSHAKE)
    {
        cc_handshake_mod_t *handshake = data_struct;

        // random id
        handshake->random_id = *((uint16_t *) pdata);
        pdata += sizeof(uint16_t);

        handshake->status = *pdata++;
        handshake->device_id = *pdata++;
        handshake->channel = *pdata++;
    }
    if (msg->command == CC_CMD_DEV_CONTROL)
    {
        int *enable = data_struct;
        *enable = *pdata++;
    }
    else if (msg->command == CC_CMD_ASSIGNMENT)
    {
        cc_assignment_t *assignment = data_struct;

        // assignment id, actuator id
        assignment->id = *pdata++;
        assignment->actuator_id = *pdata++;

        // assignment label
        pdata += str16_deserialize(pdata, &assignment->label);

        // value, min, max, def
        pdata += bytes_to_float(pdata, &assignment->value);
        pdata += bytes_to_float(pdata, &assignment->min);
        pdata += bytes_to_float(pdata, &assignment->max);
        pdata += bytes_to_float(pdata, &assignment->def);

        // mode
        uint8_t *pmode = (uint8_t *) &assignment->mode;
        *pmode++ = *pdata++;
        *pmode++ = *pdata++;
        *pmode++ = *pdata++;
        *pmode++ = *pdata++;

        // steps
        uint8_t *psteps = (uint8_t *) &assignment->steps;
        *psteps++ = *pdata++;
        *psteps++ = *pdata++;

        // unit
        pdata += str16_deserialize(pdata, &assignment->unit);
    }
    else if (msg->command == CC_CMD_UNASSIGNMENT)
    {
        uint8_t *assignment_id = data_struct;

        *assignment_id = *pdata++;
    }

    return 0;
}

int cc_msg_builder(int command, const void *data_struct, cc_msg_t *msg)
{
    uint8_t *pdata = msg->data;
    msg->command = command;

    if (command == CC_CMD_HANDSHAKE)
    {
        const cc_handshake_t *handshake = data_struct;

        // serialize uri
        pdata += string_serialize(handshake->uri, pdata);

        // random id
        uint8_t *pvalue = (uint8_t *) &handshake->random_id;
        *pdata++ = *pvalue++;
        *pdata++ = *pvalue++;

        // protocol version
        *pdata++ = handshake->protocol.major;
        *pdata++ = handshake->protocol.minor;

        // firmware version
        *pdata++ = handshake->firmware.major;
        *pdata++ = handshake->firmware.minor;
        *pdata++ = handshake->firmware.micro;
    }
    else if (command == CC_CMD_DEV_DESCRIPTOR)
    {
        const cc_device_t *device = data_struct;

        // serialize label
        pdata += string_serialize(device->label, pdata);

        // number of actuators
        *pdata++ = device->actuators_count;

        // serialize actuators data
        for (unsigned int i = 0; i < device->actuators_count; i++)
        {
            cc_actuator_t *actuator = device->actuators[i];

            // actuator name
            pdata += str16_serialize(&actuator->name, pdata);

            // supported modes
            uint8_t *modes = (uint8_t *) &actuator->supported_modes;
            *pdata++ = *modes++;
            *pdata++ = *modes++;
            *pdata++ = *modes++;
            *pdata++ = *modes++;

            // max assignments
            *pdata++ = actuator->max_assignments;
        }
    }
    else if (command == CC_CMD_ASSIGNMENT || command == CC_CMD_UNASSIGNMENT)
    {
        // no data
    }
    else if (command == CC_CMD_DATA_UPDATE)
    {
        cc_updates_t *updates = cc_updates();
        *pdata++ = updates->count;

        // serialize updates data
        cc_update_t update;
        while (cc_update_pop(&update))
        {
            uint8_t *pvalue = (uint8_t *) &update.value;

            *pdata++ = update.assignment_id;
            *pdata++ = *pvalue++;
            *pdata++ = *pvalue++;
            *pdata++ = *pvalue++;
            *pdata++ = *pvalue++;
        }
    }
    else
    {
        return -1;
    }

    msg->data_size = (pdata - msg->data);

    return 0;
}
