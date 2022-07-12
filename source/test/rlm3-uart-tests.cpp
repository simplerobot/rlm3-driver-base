#include "Test.hpp"
#include "rlm3-uart.h"
#include "rlm3-task.h"
#include "main.h"
#include <cctype>
#include "logger.h"

#include <vector>


LOGGER_ZONE(TEST);


TEST_CASE(UART2_Lifecycle_HappyCase)
{
	ASSERT(!RLM3_UART2_IsInit());
	RLM3_UART2_Init();
	ASSERT(RLM3_UART2_IsInit());
	RLM3_UART2_Deinit();
	ASSERT(!RLM3_UART2_IsInit());
}

TEST_CASE(UART4_Lifecycle_HappyCase)
{
	ASSERT(!RLM3_UART4_IsInit());
	RLM3_UART4_Init();
	ASSERT(RLM3_UART4_IsInit());
	RLM3_UART4_Deinit();
	ASSERT(!RLM3_UART4_IsInit());
}

TEST_CASE(UART2_Transmit_HappyCase)
{
	RLM3_UART2_Init();
	bool result = RLM3_UART2_Transmit((const uint8_t*)"ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n", 28);
	RLM3_UART2_Deinit();

	ASSERT(result);
}

TEST_CASE(UART4_Transmit_HappyCase)
{
	RLM3_UART4_Init();
	bool result = RLM3_UART4_Transmit((const uint8_t*)"ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n", 28);
	RLM3_UART4_Deinit();

	ASSERT(result);
}

TEST_CASE(UART_Receive_HappyCase)
{
	RLM3_UART2_Init();

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
	ASSERT(RLM3_UART2_Transmit(command, sizeof(command)));

	RLM3_Time start_time = RLM3_GetCurrentTime();
	std::vector<uint8_t> buffer;
	buffer.reserve(1024);
	while (RLM3_GetCurrentTime() - start_time < 500)
	{
		const uint8_t* data;
		size_t size;
		bool result = RLM3_UART2_ReceiveWithTimeout(&data, &size, 1000);
		if (result)
			for (size_t i = 0; i < size; i++)
				buffer.push_back(data[i]);
	}

	RLM3_UART2_Deinit();

	ASSERT(buffer.size() == 18);
	ASSERT(buffer == std::vector<uint8_t>({ 0xA0, 0xA1, 0x00, 0x02, 0x83, 0x10, 0x93, 0x0D, 0x0A, 0xA0, 0xA1, 0x00, 0x02, 0x86, 0x01, 0x87, 0x0D, 0x0A }));
}

/*
extern void RLM3_UART4_Init();
extern void RLM3_UART4_Deinit();
extern bool RLM3_UART4_IsInit();

extern bool RLM3_UART4_Transmit(const uint8_t* data, size_t size);
extern bool RLM3_UART4_Receive(const uint8_t** data, size_t* size);

 */
