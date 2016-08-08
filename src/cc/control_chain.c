/*
************************************************************************************************************************
*       INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "serial.h"
#include "utils.h"
#include "msg.h"
#include "handshake.h"
#include "device.h"
#include "chip.h"


/*
************************************************************************************************************************
*       INTERNAL MACROS
************************************************************************************************************************
*/

#define BAUD_RATE           115200
#define SYNC_BYTE           0xA7
#define HEADER_SIZE         4
#define BROADCAST_ADDRESS   0
#define DATA_BUFFER_SIZE    128


/*
************************************************************************************************************************
*       INTERNAL CONSTANTS
************************************************************************************************************************
*/

// communication states
enum {WAITING_SYNCING, WAITING_HANDSHAKE, WAITING_DEV_DESCRIPTOR, LISTENING_REQUESTS};


/*
************************************************************************************************************************
*       INTERNAL DATA TYPES
************************************************************************************************************************
*/

typedef struct cc_handle_t {
    int comm_state, msg_state;
    serial_t *serial;
    cc_msg_t *msg;
    uint8_t address;
    uint8_t *tx_buffer;
    cc_handshake_t *handshake;
} cc_handle_t;


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static cc_handle_t g_cc_handle;
static cc_msg_t g_cc_msg;
static uint8_t g_msg_buffer[DATA_BUFFER_SIZE];
static uint8_t g_tx_buffer[DATA_BUFFER_SIZE];


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/

static void send(cc_handle_t *handle, const cc_msg_t *msg)
{
    uint32_t i = 0;
    uint8_t *buffer = handle->tx_buffer;

    // sync byte and header
    buffer[i++] = SYNC_BYTE;
    buffer[i++] = handle->address;
    buffer[i++] = msg->command;
    buffer[i++] = (msg->data_size >> 0) & 0xFF;
    buffer[i++] = (msg->data_size >> 8) & 0xFF;

    // data
    if (msg->data_size > 0)
    {
        uint32_t j = 0;
        for (j = 0; j < msg->data_size; j++)
        {
            buffer[i++] = msg->data[j];
        }
    }

    // calculate crc skipping sync byte
    buffer[i++] = crc8(&buffer[1], HEADER_SIZE + msg->data_size);

    // send message
    serial_data_t sdata;
    sdata.data = buffer;
    sdata.size = i;
    serial_send(handle->serial, &sdata);
}

static void parser(cc_handle_t *handle)
{
    cc_msg_t *msg = handle->msg;

    if (handle->comm_state == WAITING_SYNCING)
    {
        if (msg->command == CC_CMD_CHAIN_SYNC)
        {
            handle->handshake = cc_handshake();

            // build and send handshake message
            cc_msg_builder(CC_CMD_HANDSHAKE, handle->handshake, msg);
            send(handle, msg);

            handle->comm_state++;
        }
    }
    else if (handle->comm_state == WAITING_HANDSHAKE)
    {
        if (msg->command == CC_CMD_HANDSHAKE)
        {
            cc_handshake_t handshake;
            cc_msg_parser(msg, &handshake);

            // check whether master replied to this device
            if (handle->handshake->random_id == handshake.random_id)
            {
                // TODO: check protocol version
                handle->address = msg->dev_address;
                handle->comm_state++;
            }
        }
    }
    else if (handle->comm_state == WAITING_DEV_DESCRIPTOR)
    {
        if (msg->command == CC_CMD_DEV_DESCRIPTOR)
        {
            cc_dev_descriptor_t *desc;
            desc = cc_device_descriptor();

            cc_msg_builder(CC_CMD_DEV_DESCRIPTOR, desc, msg);
            send(handle, msg);

            // device assumes message was successfully delivered
            handle->comm_state++;
        }
    }
    else if (handle->comm_state == LISTENING_REQUESTS)
    {
        if (msg->command == CC_CMD_CHAIN_SYNC)
        {
        }
        else if (msg->command == CC_CMD_ASSIGNMENT)
        {
Chip_GPIO_SetPinState(LPC_GPIO, 0, 14, 0);
            cc_assignment_t assignment;
            cc_msg_parser(msg, &assignment);
            cc_assignment_add(&assignment);

            cc_msg_builder(CC_CMD_ASSIGNMENT, NULL, msg);
            send(handle, msg);
        }
        else if (msg->command == CC_CMD_UNASSIGNMENT)
        {
        }

    }
}

static void serial_recv(void *arg)
{
    serial_data_t *serial = arg;
    uint16_t count = 0;

    cc_handle_t *handle = &g_cc_handle;
    cc_msg_t *msg = handle->msg;

    while (serial->size--)
    {
        uint8_t byte = serial->data[count++];
        uint16_t data_size;

        // store header bytes
        if (handle->msg_state > 0 && handle->msg_state <= HEADER_SIZE)
            msg->header[handle->msg_state - 1] = byte;

        switch (handle->msg_state)
        {
            // sync
            case 0:
                if (byte == SYNC_BYTE)
                {
                    msg->data_idx = 0;
                    msg->data_size = 0;
                    handle->msg_state++;
                }
                break;

            // address
            case 1:
                if (byte == BROADCAST_ADDRESS ||
                    byte == handle->address   ||
                    handle->address == BROADCAST_ADDRESS)
                {
                    msg->dev_address = byte;
                    handle->msg_state++;
                }
                else
                {
                    handle->msg_state = 0;
                }
                break;

            // command
            case 2:
                msg->command = byte;
                handle->msg_state++;
                break;

            // data size LSB
            case 3:
                msg->data_size = byte;
                handle->msg_state++;
                break;

            // data size MSB
            case 4:
                data_size = byte;
                data_size <<= 8;
                data_size |= msg->data_size;
                msg->data_size = data_size;

                // sync message must have no data
                if (msg->command == CC_CMD_CHAIN_SYNC && data_size > 0)
                {
                    handle->msg_state = 0;
                }
                else
                {
                    handle->msg_state++;

                    // if no data is expected skip data retrieving step
                    if (data_size == 0)
                        handle->msg_state++;
                }
                break;

            // data
            case 5:
                msg->data[msg->data_idx++] = byte;
                if (msg->data_idx == msg->data_size)
                    handle->msg_state++;
                break;

            // crc
            case 6:
                if (crc8(msg->header, HEADER_SIZE + msg->data_size) == byte)
                    parser(handle);

                handle->msg_state = 0;
                break;
        }
    }
}


/*
************************************************************************************************************************
*       GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void cc_init(void)
{
    g_cc_handle.serial = serial_init(BAUD_RATE, serial_recv);
    g_cc_handle.tx_buffer = g_tx_buffer;
    g_cc_handle.msg = &g_cc_msg;
    g_cc_msg.header = g_msg_buffer;
    g_cc_msg.data = &g_cc_msg.header[HEADER_SIZE];
}
