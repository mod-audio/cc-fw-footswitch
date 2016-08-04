/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include "msg.h"
#include "handshake.h"
#include "device.h"


/*
************************************************************************************************************************
*       INTERNAL MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       INTERNAL CONSTANTS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       INTERNAL DATA TYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
*/

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
        *pdata++ = desc->actuators_count;
        for (int i = 0; i < desc->actuators_count; i++)
        {
            *pdata++ = desc->actuators[i]->id;
        }

        msg->data_size = (pdata - msg->data);
    }
    else if (command == CC_CMD_DATA_UPDATE)
    {
    }
    else
    {
        return -1;
    }

    return 0;
}
