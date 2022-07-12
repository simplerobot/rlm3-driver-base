#include "rlm3-uart.h"
#include "usart.h"
#include "stm32f4xx_hal_uart.h"
#include "Assert.h"
#include "logger.h"
#include "rlm3-task.h"


LOGGER_ZONE(UART);


#ifndef UART_BUFFER_SIZE
#define UART_BUFFER_SIZE 32
#endif
#ifndef UART2_BUFFER_SIZE
#define UART2_BUFFER_SIZE UART_BUFFER_SIZE
#endif
#ifndef UART4_BUFFER_SIZE
#define UART4_BUFFER_SIZE UART_BUFFER_SIZE
#endif


enum
{
	UART_STATE_OFF,
	UART_STATE_IDLE,
	UART_STATE_ERROR,
	UART_STATE_TX_WAIT,
	UART_STATE_TX_DONE,
	UART_STATE_RX_WAIT,
	UART_STATE_RX_DONE,
};

typedef struct
{
	size_t buffer_size;
	uint8_t* buffer_data;
	volatile RLM3_Task waiting_tx_thread;
	volatile RLM3_Task waiting_rx_thread;
	volatile size_t buffer_head;
	volatile size_t buffer_tail;
	volatile size_t buffer_hold;
	volatile uint8_t tx_state;
	volatile uint8_t rx_state;
} UART_INFO;

static UART_INFO g_uart2 = { 0 };
static UART_INFO g_uart4 = { 0 };

static uint8_t g_buffer_uart2[UART2_BUFFER_SIZE];
static uint8_t g_buffer_uart4[UART4_BUFFER_SIZE];


static bool UartStartRead(UART_INFO* uart, UART_HandleTypeDef* huart)
{
	ASSERT(uart->rx_state == UART_STATE_IDLE);
	ASSERT(uart->buffer_head < uart->buffer_size);
	size_t head = uart->buffer_head;
	size_t tail = uart->buffer_tail;
	size_t read_size;
	if (head >= tail)
		read_size = uart->buffer_size - head;
	else
		read_size = tail - head - 1;
	if (read_size > uart->buffer_size / 2)
		read_size = uart->buffer_size / 2;
	if (read_size == 0)
	{
		LOG_ERROR("Queue Full");
		uart->rx_state = UART_STATE_IDLE;
		return false;
	}
	uart->rx_state = UART_STATE_RX_WAIT;
	HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_IT(huart, uart->buffer_data + head, read_size);
	if (status != HAL_OK)
	{
		LOG_ERROR("Queue Fail %d", status);
		uart->rx_state = UART_STATE_IDLE;
		return false;
	}
	LOG_TRACE("Queue %d", read_size);
	return true;
}

static void UartInit(UART_INFO* uart, UART_HandleTypeDef* huart, void (*init_method)())
{
	ASSERT(uart->tx_state == UART_STATE_OFF);
	ASSERT(uart->rx_state == UART_STATE_OFF);
	ASSERT(uart->buffer_data != NULL);
	ASSERT(uart->buffer_size > 0);

	LOG_TRACE("Init");

	uart->waiting_tx_thread = NULL;
	uart->waiting_rx_thread = NULL;
	uart->buffer_head = 0;
	uart->buffer_tail = 0;
	uart->buffer_hold = 0;
	uart->tx_state = UART_STATE_IDLE;
	uart->rx_state = UART_STATE_IDLE;

	init_method();

	UartStartRead(uart, huart);
}

static void UartDeinit(UART_INFO* uart, UART_HandleTypeDef* huart)
{
	ASSERT(uart->tx_state != UART_STATE_OFF);
	ASSERT(uart->rx_state != UART_STATE_OFF);

	LOG_TRACE("Deinit");

	HAL_UART_DeInit(huart);

	uart->tx_state = UART_STATE_OFF;
	uart->rx_state = UART_STATE_OFF;
}

static bool UartTransmit(UART_INFO* uart, UART_HandleTypeDef* huart, const uint8_t* data, size_t size)
{
	ASSERT(uart->tx_state == UART_STATE_IDLE);
	ASSERT(uart->waiting_tx_thread == NULL);

	LOG_TRACE("TX %d", (int)size);

	uart->waiting_tx_thread = RLM3_GetCurrentTask();
	uart->tx_state = UART_STATE_TX_WAIT;

	HAL_StatusTypeDef status = HAL_UART_Transmit_IT(huart, data, size);
	while (status == HAL_OK && uart->tx_state == UART_STATE_TX_WAIT)
		RLM3_Take();
	bool result = (status == HAL_OK && uart->tx_state == UART_STATE_TX_DONE);

	uart->waiting_tx_thread = NULL;
	uart->tx_state = UART_STATE_IDLE;

	return result;
}

static bool UartFetchFromBuffer(UART_INFO* uart, UART_HandleTypeDef* huart, const uint8_t** data_out, size_t* size_out)
{
	size_t head = uart->buffer_head;
	size_t tail = uart->buffer_tail;
	size_t available_size;
	if (head >= tail)
		available_size = head - tail;
	else
		available_size = uart->buffer_size - tail;
	if (available_size > uart->buffer_size / 2)
		available_size = uart->buffer_size / 2;
	if (available_size == 0)
	{
		*size_out = 0;
		return false;
	}
	*data_out = uart->buffer_data + uart->buffer_tail;
	*size_out = available_size;

	uart->buffer_hold = uart->buffer_tail + available_size;
	if (uart->buffer_hold >= uart->buffer_size)
		uart->buffer_hold = 0;

	return true;
}

static bool UartReceive(UART_INFO* uart, UART_HandleTypeDef* huart, const uint8_t** data_out, size_t* size_out)
{
	ASSERT(uart->waiting_rx_thread == NULL);

	LOG_TRACE("RX");

	uart->waiting_rx_thread = RLM3_GetCurrentTask();

	// Immediately release previous hold and start reading more data if we had run out of space.
	uart->buffer_tail = uart->buffer_hold;
	if (uart->rx_state == UART_STATE_IDLE)
		UartStartRead(uart, huart);

	// If we already have data in our buffer, just return it.
	bool result = UartFetchFromBuffer(uart, huart, data_out, size_out);


	// Otherwise, wait to get more data from the interrupt handler.
	if (!result)
	{
		RLM3_Take();
		result = UartFetchFromBuffer(uart, huart, data_out, size_out);
	}

	uart->waiting_rx_thread = NULL;
	return result;
}

static bool UartReceiveWithTimeout(UART_INFO* uart, UART_HandleTypeDef* huart, const uint8_t** data_out, size_t* size_out, size_t timeout_ms)
{
	ASSERT(uart->waiting_rx_thread == NULL);

	LOG_TRACE("RX %d", (int)timeout_ms);

	uart->waiting_rx_thread = RLM3_GetCurrentTask();

	// Immediately release previous hold and start reading more data if we had run out of space.
	uart->buffer_tail = uart->buffer_hold;
	if (uart->rx_state == UART_STATE_IDLE)
		UartStartRead(uart, huart);

	// If we already have data in our buffer, just return it.
	bool result = UartFetchFromBuffer(uart, huart, data_out, size_out);


	// Otherwise, wait to get more data from the interrupt handler.
	if (!result)
	{
		RLM3_TakeTimeout(timeout_ms);
		result = UartFetchFromBuffer(uart, huart, data_out, size_out);
	}

	uart->waiting_rx_thread = NULL;
	return result;
}

extern void RLM3_UART2_Init()
{
	g_uart2.buffer_size = UART2_BUFFER_SIZE;
	g_uart2.buffer_data = g_buffer_uart2;
	UartInit(&g_uart2, &huart2, MX_USART2_UART_Init);
}

extern void RLM3_UART2_Deinit()
{
	UartDeinit(&g_uart2, &huart2);
}

extern bool RLM3_UART2_IsInit()
{
	return (g_uart2.tx_state != UART_STATE_OFF);
}

extern bool RLM3_UART2_Transmit(const uint8_t* data, size_t size)
{
	return UartTransmit(&g_uart2, &huart2, data, size);
}

extern bool RLM3_UART2_Receive(const uint8_t** data_out, size_t* size_out)
{
	return UartReceive(&g_uart2, &huart2, data_out, size_out);

}

extern bool RLM3_UART2_ReceiveWithTimeout(const uint8_t** data_out, size_t* size_out, size_t timeout_ms)
{
	return UartReceiveWithTimeout(&g_uart2, &huart2, data_out, size_out, timeout_ms);
}

extern void RLM3_UART4_Init()
{
	g_uart4.buffer_size = UART4_BUFFER_SIZE;
	g_uart4.buffer_data = g_buffer_uart4;
	UartInit(&g_uart4, &huart4, MX_UART4_Init);
}

extern void RLM3_UART4_Deinit()
{
	UartDeinit(&g_uart4, &huart4);
}

extern bool RLM3_UART4_IsInit()
{
	return (g_uart4.tx_state != UART_STATE_OFF);
}

extern bool RLM3_UART4_Transmit(const uint8_t* data, size_t size)
{
	return UartTransmit(&g_uart4, &huart4, data, size);
}

extern bool RLM3_UART4_Receive(const uint8_t** data_out, size_t* size_out)
{
	return UartReceive(&g_uart4, &huart4, data_out, size_out);
}

extern bool RLM3_UART4_ReceiveWithTimeout(const uint8_t** data_out, size_t* size_out, size_t timeout_ms)
{
	return UartReceiveWithTimeout(&g_uart4, &huart4, data_out, size_out, timeout_ms);
}

static void WakeupWaitingTxThreadFromISR(UART_HandleTypeDef *huart, uint8_t new_state, const char* type)
{
	LOG_TRACE("ISR %s", type);

	UART_INFO* uart = NULL;
	if (huart == &huart2)
		uart = &g_uart2;
	if (huart == &huart4)
		uart = &g_uart4;

	if (uart != NULL)
	{
		uart->tx_state = new_state;
		RLM3_Give(uart->waiting_tx_thread);
	}
}

extern void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	WakeupWaitingTxThreadFromISR(huart, UART_STATE_TX_DONE, "TX");
}

extern void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	// TODO: Is there a way to tell if the error is from transmit or receive?
	WakeupWaitingTxThreadFromISR(huart, UART_STATE_ERROR, "ER");
}

extern void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
	LOG_TRACE("ISR EVT %d", (int)size);

	UART_INFO* uart = NULL;
	if (huart == &huart2)
		uart = &g_uart2;
	if (huart == &huart4)
		uart = &g_uart4;

	if (uart != NULL)
	{
		ASSERT(uart->buffer_head < uart->buffer_size);
		ASSERT(uart->buffer_head + size <= uart->buffer_size);

		uart->rx_state = UART_STATE_IDLE;
		uart->buffer_head += size;
		if (uart->buffer_head >= uart->buffer_size)
			uart->buffer_head = 0;
		UartStartRead(uart, huart);
		RLM3_Give(uart->waiting_rx_thread);
	}
}

extern void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	LOG_ALWAYS("ISR RX %d ODD **", (int)huart->RxXferCount);
}
