#include "Test.hpp"
#include "rlm3-memory.h"
#include "rlm3-task.h"
#include "logger.h"
#include <random>


LOGGER_ZONE(TEST_MEM);


TEST_CASE(MEMORY_Lifecycle)
{
	ASSERT(!RLM3_MEMORY_IsInit());
	RLM3_MEMORY_Init();
	ASSERT(RLM3_MEMORY_IsInit());
	RLM3_MEMORY_Deinit();
	ASSERT(!RLM3_MEMORY_IsInit());
}

TEST_CASE(MEMORY_HappyCase)
{
	RLM3_MEMORY_Init();

	volatile uint32_t* test = (uint32_t*)RLM3_EXTERNAL_MEMORY_ADDRESS;
	*test = 0x12345678;

	ASSERT(*test == 0x12345678);

	RLM3_MEMORY_Deinit();
}

TEST_CASE(MEMORY_WriteAllSequentialWords)
{
	std::default_random_engine random(20220803);

	RLM3_MEMORY_Init();

	RLM3_Time write_start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < RLM3_EXTERNAL_MEMORY_SIZE; i += 4)
		*(uint32_t*)(RLM3_EXTERNAL_MEMORY_ADDRESS + i) = random();
	RLM3_Time write_finish_time = RLM3_GetCurrentTime();

	random.seed(20220803);

	RLM3_Time read_start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < RLM3_EXTERNAL_MEMORY_SIZE; i += 4)
		ASSERT(*(const uint32_t*)(RLM3_EXTERNAL_MEMORY_ADDRESS + i) == random());
	RLM3_Time read_finish_time = RLM3_GetCurrentTime();

	RLM3_MEMORY_Deinit();

	LOG_ALWAYS("Write Time: %u ms", (int)(write_finish_time - write_start_time));
	LOG_ALWAYS("Read Time: %u ms", (int)(read_finish_time - read_start_time));
}

static size_t LCG_Next(size_t x)
{
	// Use an LCG to generate the each memory address in a "random" order.  x = (a * x + c) mod m
	size_t c = 7919; // Relatively prime to m
	size_t a = 33;
	size_t m = RLM3_EXTERNAL_MEMORY_SIZE;

	return (a * x + c) % m;
}

TEST_CASE(MEMORY_WriteRandomBytes)
{
	std::default_random_engine random(20220803);

	RLM3_MEMORY_Init();

	RLM3_Time write_start_time = RLM3_GetCurrentTime();
	for (size_t i = 0, x = 1; i < RLM3_EXTERNAL_MEMORY_SIZE; i++)
	{
		x = LCG_Next(x);
		*(RLM3_EXTERNAL_MEMORY_ADDRESS + x) = (uint8_t)random();
	}
	RLM3_Time write_finish_time = RLM3_GetCurrentTime();

	random.seed(20220803);

	RLM3_Time read_start_time = RLM3_GetCurrentTime();
	for (size_t i = 0, x = 1; i < RLM3_EXTERNAL_MEMORY_SIZE; i++)
	{
		x = LCG_Next(x);
		ASSERT(*(RLM3_EXTERNAL_MEMORY_ADDRESS + x) == (uint8_t)random());
	}
	RLM3_Time read_finish_time = RLM3_GetCurrentTime();

	RLM3_MEMORY_Deinit();

	LOG_ALWAYS("Write Time: %u ms", (int)(write_finish_time - write_start_time));
	LOG_ALWAYS("Read Time: %u ms", (int)(read_finish_time - read_start_time));
}

