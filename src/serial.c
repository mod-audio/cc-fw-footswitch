/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "chip.h"
#include "serial.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define RX_BUFFER_SIZE  64


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

typedef struct serial_t {
    RINGBUFF_T rx_rb;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    void (*receive_cb)(void *arg);
} serial_t;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static serial_t g_serial;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

void UART_IRQHandler(void)
{
    serial_t *serial = &g_serial;

    // TODO: handle errors

    // use default ring buffer handler
    Chip_UART_RXIntHandlerRB(LPC_USART, &serial->rx_rb);

    uint8_t buffer[RX_BUFFER_SIZE], read;
    read = Chip_UART_ReadRB(LPC_USART, &serial->rx_rb, &buffer, sizeof(buffer));

    if (read > 0 && serial->receive_cb)
    {
        serial_data_t sdata;
        sdata.data = buffer;
        sdata.size = read;
        serial->receive_cb(&sdata);
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

serial_t* serial_init(uint32_t baud_rate, void (*receive_cb)(void *arg))
{
    // this cpu has only one USART
    serial_t *serial = &g_serial;

    // configure pins function
    Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 13, FUNC3);  // TX pin
    Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 14, FUNC3);  // RX pin
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 16);     // OE pin
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 16, 1);

    // setup serial
    Chip_UART_Init(LPC_USART);
    Chip_UART_SetBaud(LPC_USART, baud_rate);
    Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
    Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
    Chip_UART_TXEnable(LPC_USART);

    // enable receive data and line status interrupt
    Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

    // preemption = 1, sub-priority = 1
    NVIC_SetPriority(UART0_IRQn, 1);
    NVIC_EnableIRQ(UART0_IRQn);

    // create ring buffers
    RingBuffer_Init(&serial->rx_rb, &serial->rx_buffer, 1, RX_BUFFER_SIZE);

    // set serial callback
    serial->receive_cb = receive_cb;

    return serial;
}

void serial_send(serial_t *serial, serial_data_t *sdata)
{
    if (sdata->size > 0)
        Chip_UART_SendBlocking(LPC_USART, sdata->data, sdata->size);
}
