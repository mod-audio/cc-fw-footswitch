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
    if (msg->command == CC_CMD_HANDSHAKE)
    {
        cc_handshake_t *handshake = data_struct;
        handshake->random_id = *((uint16_t *) &msg->data[0]);
        handshake->protocol.major = msg->data[2];
        handshake->protocol.minor = msg->data[3];
        handshake->protocol.micro = 0;
    }
    else if (msg->command == CC_CMD_ASSIGNMENT)
    {
        cc_assignment_t *assignment = data_struct;

        int i = 0;
        assignment->id = msg->data[i++];
        assignment->actuator_id = msg->data[i++];

        i += bytes_to_float(&msg->data[i], &assignment->value);
        i += bytes_to_float(&msg->data[i], &assignment->min);
        i += bytes_to_float(&msg->data[i], &assignment->max);
        i += bytes_to_float(&msg->data[i], &assignment->def);
        assignment->mode = msg->data[i++];
    }
    else if (msg->command == CC_CMD_UNASSIGNMENT)
    {
        uint8_t *assignment_id = data_struct;
        *assignment_id = msg->data[0];
    }

    return 0;
}

int cc_msg_builder(int command, const void *data_struct, cc_msg_t *msg)
{
    msg->command = command;

    if (command == CC_CMD_HANDSHAKE)
    {
        const cc_handshake_t *handshake = data_struct;
        uint16_t *random_id = (uint16_t *) &msg->data[0];
        *random_id = handshake->random_id;
        msg->data[2] = handshake->protocol.major;
        msg->data[3] = handshake->protocol.minor;

        msg->data_size = 4;
    }
    else if (command == CC_CMD_DEV_DESCRIPTOR)
    {
        const cc_dev_descriptor_t *desc = data_struct;
        uint8_t *pdata = msg->data;

        // serialize label
        pdata += string_serialize(desc->label, pdata);

        // serialize actuators data
        cc_actuators_t *actuators = cc_actuators();
        *pdata++ = actuators->count;
        LILI_FOREACH(actuators, node)
        {
            cc_actuator_t *actuator = node->data;
            *pdata++ = actuator->id;
        }

        msg->data_size = (pdata - msg->data);
    }
    else if (command == CC_CMD_ASSIGNMENT || command == CC_CMD_UNASSIGNMENT)
    {
        msg->data_size = 0;
    }
    else if (command == CC_CMD_DATA_UPDATE)
    {
        uint8_t *pdata = msg->data;

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

        msg->data_size = (pdata - msg->data);
    }
    else
    {
        return -1;
    }

    return 0;
}
