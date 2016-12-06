/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "control_chain.h"
#include "utils.h"
#include "msg.h"
#include "handshake.h"
#include "device.h"
#include "update.h"
#include "timer.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define SYNC_BYTE           0xA7
#define BROADCAST_ADDRESS   0


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

// communication states
enum {WAITING_SYNCING, WAITING_HANDSHAKE, WAITING_DEV_DESCRIPTOR, LISTENING_REQUESTS};

// sync message cycles definition
enum {CC_SYNC_REGULAR_CYCLE, CC_SYNC_HANDSHAKE_CYCLE};

// control chain handle struct
typedef struct cc_handle_t {
    void (*response_cb)(void *arg);
    int comm_state, msg_state;
    cc_msg_t *msg_rx, *msg_tx;
    uint8_t address;
    cc_handshake_t *handshake;
} cc_handle_t;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static cc_handle_t g_cc_handle;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static void send(cc_handle_t *handle, const cc_msg_t *msg)
{
    uint8_t *buffer = handle->msg_tx->header;

    // add sync byte
    buffer[0] = SYNC_BYTE;
    buffer++;

    // header
    uint32_t i = 0;
    buffer[i++] = handle->address;
    buffer[i++] = msg->command;
    buffer[i++] = (msg->data_size >> 0) & 0xFF;
    buffer[i++] = (msg->data_size >> 8) & 0xFF;

    // data
    if (msg->data_size > 0)
    {
        if (msg != handle->msg_tx)
        {
            for (uint32_t j = 0; j < msg->data_size; j++)
                buffer[i++] = msg->data[j];
        }
        else
        {
            i += msg->data_size;
        }
    }

    // calculate crc
    buffer[i++] = crc8(buffer, CC_MSG_HEADER_SIZE + msg->data_size);

    // send message
    cc_data_t response;
    response.data = buffer;
    response.size = i + 1;
    handle->response_cb(&response);
}

static void parser(cc_handle_t *handle)
{
    cc_msg_t *msg_rx = handle->msg_rx;

    if (handle->comm_state == WAITING_SYNCING)
    {
        if (msg_rx->command == CC_CMD_CHAIN_SYNC)
        {
            // check if it's handshake sync cycle
            int sync_cycle = msg_rx->data[0];
            if (sync_cycle != CC_SYNC_HANDSHAKE_CYCLE)
                return;

            handle->handshake = cc_handshake();

            // build and send handshake message
            cc_msg_builder(CC_CMD_HANDSHAKE, handle->handshake, handle->msg_tx);
            send(handle, handle->msg_tx);

            handle->comm_state++;
        }
    }
    else if (handle->comm_state == WAITING_HANDSHAKE)
    {
        if (msg_rx->command == CC_CMD_HANDSHAKE)
        {
            cc_handshake_t handshake;
            cc_msg_parser(msg_rx, &handshake);

            // check whether master replied to this device
            if (handle->handshake->random_id == handshake.random_id)
            {
                // TODO: check protocol version
                handle->address = msg_rx->dev_address;
                handle->comm_state++;
            }
        }
    }
    else if (handle->comm_state == WAITING_DEV_DESCRIPTOR)
    {
        if (msg_rx->command == CC_CMD_DEV_DESCRIPTOR)
        {
            cc_dev_descriptor_t *desc;
            desc = cc_device_descriptor("FootEx");

            cc_msg_builder(CC_CMD_DEV_DESCRIPTOR, desc, handle->msg_tx);
            send(handle, handle->msg_tx);

            // device assumes message was successfully delivered
            handle->comm_state++;
        }
    }
    else if (handle->comm_state == LISTENING_REQUESTS)
    {
        if (msg_rx->command == CC_CMD_CHAIN_SYNC)
        {
            // device address is used to define the communication frame
            // timer is reseted each sync message
            if (cc_assignments())
                timer_set(handle->address);
        }
        else if (msg_rx->command == CC_CMD_ASSIGNMENT)
        {
            cc_assignment_t assignment;
            cc_msg_parser(msg_rx, &assignment);
            cc_assignment_add(&assignment);

            cc_msg_builder(CC_CMD_ASSIGNMENT, 0, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
        else if (msg_rx->command == CC_CMD_UNASSIGNMENT)
        {
            uint8_t assignment_id;
            cc_msg_parser(msg_rx, &assignment_id);
            cc_assignment_remove(assignment_id);

            cc_msg_builder(CC_CMD_UNASSIGNMENT, 0, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
    }
}

static void timer_callback(void)
{
    cc_handle_t *handle = &g_cc_handle;

    // TODO: [future/optimization] the update message shouldn't be built in the interrupt
    // handler the time of the frame is being wasted with processing. ideally it has to be
    // cached in the main loop and the interrupt handler is only used to queue the message
    // (send command)

    cc_updates_t *updates = cc_updates();
    if (!updates || updates->count == 0)
        return;

    cc_msg_builder(CC_CMD_DATA_UPDATE, 0, handle->msg_tx);
    send(handle, handle->msg_tx);
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void cc_init(void (*response_cb)(void *arg))
{
    g_cc_handle.response_cb = response_cb;
    g_cc_handle.msg_rx = cc_msg_new();
    g_cc_handle.msg_tx = cc_msg_new();

    timer_init(timer_callback);
}

void cc_process(void)
{
    // process each actuator going through all assignments
    // data update messages will be queued and sent in the next frame
    cc_actuators_process();
}

void cc_parse(const cc_data_t *received)
{
    uint16_t count = 0;

    cc_handle_t *handle = &g_cc_handle;
    cc_msg_t *msg = handle->msg_rx;

    uint32_t size = received->size;
    while (size--)
    {
        uint8_t byte = received->data[count++];
        uint16_t data_size;

        // store header bytes
        if (handle->msg_state > 0 && handle->msg_state <= CC_MSG_HEADER_SIZE)
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

                handle->msg_state++;

                // if no data is expected skip data retrieving step
                if (data_size == 0)
                    handle->msg_state++;
                break;

            // data
            case 5:
                msg->data[msg->data_idx++] = byte;
                if (msg->data_idx == msg->data_size)
                    handle->msg_state++;
                break;

            // crc
            case 6:
                if (crc8(msg->header, CC_MSG_HEADER_SIZE + msg->data_size) == byte)
                    parser(handle);

                handle->msg_state = 0;
                break;
        }
    }
}
