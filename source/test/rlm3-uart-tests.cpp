#include "Test.hpp"
#include "rlm3-uart.h"
#include "rlm3-task.h"
#include "main.h"
#include <cctype>
#include "logger.h"

#include <vector>


LOGGER_ZONE(TEST);


static volatile RLM3_Task g_uart2_task = nullptr;
static volatile uint8_t* g_uart2_buffer_tx = nullptr;
static volatile uint8_t* g_uart2_buffer_rx = nullptr;
static volatile size_t g_uart2_size_tx = 0;
static volatile size_t g_uart2_size_rx = 0;
static volatile size_t g_uart2_error_count = 0;

static volatile RLM3_Task g_uart4_task = nullptr;
static volatile uint8_t* g_uart4_buffer_tx = nullptr;
static volatile uint8_t* g_uart4_buffer_rx = nullptr;
static volatile size_t g_uart4_size_tx = 0;
static volatile size_t g_uart4_size_rx = 0;
static volatile size_t g_uart4_error_count = 0;


TEST_CASE(UART2_Lifecycle_HappyCase)
{
	ASSERT(!RLM3_UART2_IsInit());
	RLM3_UART2_Init(115200);
	ASSERT(RLM3_UART2_IsInit());
	RLM3_UART2_Deinit();
	ASSERT(!RLM3_UART2_IsInit());
}

TEST_CASE(UART4_Lifecycle_HappyCase)
{
	ASSERT(!RLM3_UART4_IsInit());
	RLM3_UART4_Init(115200);
	ASSERT(RLM3_UART4_IsInit());
	RLM3_UART4_Deinit();
	ASSERT(!RLM3_UART4_IsInit());
}

TEST_CASE(UART2_Transmit_HappyCase)
{
	uint8_t buffer[26];
	for (size_t i = 0; i < 26; i++)
		buffer[i] = 'A' + i;
	RLM3_UART2_Init(115200);

	g_uart2_task = RLM3_GetCurrentTask();
	g_uart2_buffer_tx = buffer;
	g_uart2_size_tx = 26;

	RLM3_UART2_EnsureTransmit();
	for (size_t i = 0; i < 10 && g_uart2_size_tx != 0; i++)
		RLM3_TakeWithTimeout(0);
	g_uart2_task = nullptr;

	RLM3_UART2_Deinit();

	ASSERT(g_uart2_size_tx == 0);
	ASSERT(g_uart2_error_count == 0);
}

TEST_CASE(UART4_Transmit_HappyCase)
{
	uint8_t buffer[26];
	for (size_t i = 0; i < 26; i++)
		buffer[i] = 'A' + i;
	RLM3_UART4_Init(115200);

	g_uart4_task = RLM3_GetCurrentTask();
	g_uart4_buffer_tx = buffer;
	g_uart4_size_tx = 26;

	RLM3_UART4_EnsureTransmit();
	for (size_t i = 0; i < 10 && g_uart4_size_tx != 0; i++)
		RLM3_TakeWithTimeout(0);
	g_uart4_task = nullptr;

	RLM3_UART4_Deinit();

	ASSERT(g_uart4_size_tx == 0);
	ASSERT(g_uart4_error_count == 0);
}

TEST_CASE(UART2_Receive_HappyCase)
{
	RLM3_UART2_Init(115200);

	// Enable GPS Reset Pin
	__HAL_RCC_GPIOB_CLK_ENABLE();
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_RESET);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPS_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPS_RESET_GPIO_Port, &GPIO_InitStruct);

	// Reset the GPS module.
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_RESET);
	RLM3_Delay(10);
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port, GPS_RESET_Pin, GPIO_PIN_SET);
	RLM3_Delay(1000);

	uint8_t command[] = { 0xA0, 0xA1, 0x00, 0x01, 0x10, 0x10, 0x0D, 0x0A }; // Query position update rate.
	g_uart2_buffer_tx = command;
	g_uart2_size_tx = sizeof(command);
	RLM3_UART2_EnsureTransmit();

	uint8_t buffer[32];
	g_uart2_buffer_rx = buffer;
	g_uart2_size_rx = 32;

	RLM3_Delay(500);

	RLM3_UART2_Deinit();

	size_t count = 32 - g_uart2_size_rx;

	ASSERT(count >= 9);
	const uint8_t expected[9] = { 0xA0, 0xA1, 0x00, 0x02, 0x83, 0x10, 0x93, 0x0D, 0x0A };
	for (size_t i = 0; i < 9; i++)
		ASSERT(buffer[i] == expected[i]);
	ASSERT(g_uart2_error_count == 0);
}

extern void RLM3_UART2_ReceiveCallback(uint8_t data)
{
	if (g_uart2_buffer_rx != nullptr && g_uart2_size_rx != 0)
	{
		*(g_uart2_buffer_rx++) = data;
		if (--g_uart2_size_rx == 0)
		{
			g_uart2_buffer_rx = nullptr;
			RLM3_GiveFromISR(g_uart2_task);
		}
	}
}

extern bool RLM3_UART2_TransmitCallback(uint8_t* data_to_send)
{
	if (g_uart2_buffer_tx == nullptr || g_uart2_size_tx == 0)
		return false;
	*data_to_send = *(g_uart2_buffer_tx++);
	if (--g_uart2_size_tx == 0)
	{
		g_uart2_buffer_tx = nullptr;
		RLM3_GiveFromISR(g_uart2_task);
	}
	return true;
}

extern void RLM3_UART2_ErrorCallback(uint32_t status_flags)
{
	g_uart2_error_count++;
}

extern void RLM3_UART4_ReceiveCallback(uint8_t data)
{
	if (g_uart4_buffer_rx != nullptr && g_uart4_size_rx != 0)
	{
		*(g_uart4_buffer_rx++) = data;
		if (--g_uart4_size_rx == 0)
		{
			g_uart4_buffer_rx = nullptr;
			RLM3_GiveFromISR(g_uart4_task);
		}
	}
}
extern bool RLM3_UART4_TransmitCallback(uint8_t* data_to_send)
{
	if (g_uart4_buffer_tx == nullptr || g_uart4_size_tx == 0)
		return false;
	*data_to_send = *(g_uart4_buffer_tx++);
	if (--g_uart4_size_tx == 0)
	{
		g_uart4_buffer_tx = nullptr;
		RLM3_GiveFromISR(g_uart4_task);
	}
	return true;
}

extern void RLM3_UART4_ErrorCallback(uint32_t status_flags)
{
	g_uart4_error_count++;
}
