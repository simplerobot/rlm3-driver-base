#include "rlm3-random.h"
#include "rlm3-helper.h"
#include "rlm3-task.h"
#include "logger.h"
#include "main.h"


LOGGER_ZONE(RANDOM);


static volatile RLM3_Task g_client_task = NULL;
static volatile uint8_t* g_buffer = NULL;
static volatile size_t g_size = 0;


extern void RLM3_Random_Init()
{
	__HAL_RCC_RNG_CLK_ENABLE();
    HAL_NVIC_SetPriority(HASH_RNG_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(HASH_RNG_IRQn);
}

extern void RLM3_Random_Deinit()
{
    HAL_NVIC_DisableIRQ(HASH_RNG_IRQn);
	__HAL_RCC_RNG_CLK_DISABLE();
}

extern bool RLM3_Random_IsInit()
{
	return __HAL_RCC_RNG_IS_CLK_ENABLED();
}

extern void RLM3_Random_Get(uint8_t* data, size_t size)
{
	g_client_task = RLM3_GetCurrentTask();
	g_buffer = data;
	g_size = size;
	SET_REGISTER_FLAGS(RNG->CR,
			FLAG(RNG_CR_RNGEN, 1),  // Enable Random Number Generator
			FLAG(RNG_CR_IE,    1)); // Enable Random Number Interrupt

	while (g_size > 0)
		RLM3_Take();

	SET_REGISTER_FLAGS(RNG->CR,
		FLAG(RNG_CR_RNGEN, 0),  // Disable Random Number Generator
		FLAG(RNG_CR_IE,    0)); // Disable Random Number Interrupt
	g_buffer = NULL;
	g_client_task = NULL;
}

extern void HASH_RNG_IRQHandler(void)
{
	uint32_t status = RNG->SR;
	uint32_t entropy = RNG->DR;

	if ((status & (RNG_SR_CEIS | RNG_SR_SEIS)) != 0)
	{
		if ((status & RNG_SR_CEIS) != 0)
			LOG_WARN("CLOCK ERROR");
		if ((status & RNG_SR_SEIS) != 0)
			LOG_WARN("SEED ERROR");
		SET_REGISTER_FLAGS(RNG->SR,
			FLAG(RNG_SR_CEIS, 0),  // Clear clock error
			FLAG(RNG_SR_SEIS, 0)); // Clear seed error
	}
	else if ((status & RNG_SR_DRDY) != 0 && g_size > 0)
	{
		for (size_t i = 0; i < 4 && g_size > 0; i++, g_size--)
			*(g_buffer++) = (uint8_t)(entropy >> (8 * i));
		if (g_size == 0)
			RLM3_GiveFromISR(g_client_task);
	}
}
