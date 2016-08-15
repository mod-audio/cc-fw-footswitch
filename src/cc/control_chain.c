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
#define BROADCAST_ADDRESS   0


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
    cc_msg_t *msg_rx, *msg_tx;
    uint8_t address;
    cc_handshake_t *handshake;
} cc_handle_t;


/*
************************************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static cc_handle_t g_cc_handle;


/*
************************************************************************************************************************
*       INTERNAL FUNCTIONS
************************************************************************************************************************
*/

static void timer_setup(void)
{
    // enable timer 1 clock
    Chip_TIMER_Init(LPC_TIMER32_0);

    // timer setup for match and interrupt
    Chip_TIMER_Reset(LPC_TIMER32_0);
    Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);
    Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_0, 1);

    // enable timer interrupt
    NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
}

static void timer_set(int frame)
{
    Chip_TIMER_Disable(LPC_TIMER32_0);
    Chip_TIMER_Reset(LPC_TIMER32_0);

    // timer rate is system clock rate
    uint32_t timer_freq = Chip_Clock_GetSystemClockRate();
    Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, timer_freq / (1000 / frame));

    Chip_TIMER_Enable(LPC_TIMER32_0);
}

static void send(cc_handle_t *handle, const cc_msg_t *msg)
{
    uint8_t *buffer = handle->msg_tx->header;

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

    // send sync byte
    uint8_t sync = SYNC_BYTE;
    serial_data_t sdata;
    sdata.data = &sync;
    sdata.size = 1;
    serial_send(handle->serial, &sdata);

    // send message
    sdata.data = buffer;
    sdata.size = i;
    serial_send(handle->serial, &sdata);
}

static void parser(cc_handle_t *handle)
{
    cc_msg_t *msg_rx = handle->msg_rx;

    if (handle->comm_state == WAITING_SYNCING)
    {
        if (msg_rx->command == CC_CMD_CHAIN_SYNC)
        {
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
            desc = cc_device_descriptor();

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

            cc_msg_builder(CC_CMD_ASSIGNMENT, NULL, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
        else if (msg_rx->command == CC_CMD_UNASSIGNMENT)
        {
        }
    }
}

static void serial_recv(void *arg)
{
    serial_data_t *serial = arg;
    uint16_t count = 0;

    cc_handle_t *handle = &g_cc_handle;
    cc_msg_t *msg = handle->msg_rx;

    while (serial->size--)
    {
        uint8_t byte = serial->data[count++];
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
                if (crc8(msg->header, CC_MSG_HEADER_SIZE + msg->data_size) == byte)
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

void TIMER32_0_IRQHandler(void)
{
    cc_handle_t *handle = &g_cc_handle;

    if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1))
    {
        Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
        Chip_TIMER_Disable(LPC_TIMER32_0);

        cc_assignments_t *assignments;
        for (assignments = cc_assignments(); assignments; assignments = assignments->next)
        {
            cc_assignment_t *assignment = assignments->data;

            // TODO: create flag assignment->need_update
            cc_msg_builder(CC_CMD_DATA_UPDATE, assignment, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
    }
}

void cc_init(void)
{
    g_cc_handle.serial = serial_init(BAUD_RATE, serial_recv);
    g_cc_handle.msg_rx = cc_msg_new();
    g_cc_handle.msg_tx = cc_msg_new();

    timer_setup();
}
