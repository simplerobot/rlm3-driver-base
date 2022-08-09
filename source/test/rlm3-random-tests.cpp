#include "Test.hpp"
#include "rlm3-random.h"
#include "logger.h"
#include "rlm3-task.h"
#include <algorithm>


LOGGER_ZONE(TEST_RANDOM);


TEST_CASE(Random_Lifecycle)
{
	ASSERT(!RLM3_Random_IsInit());
	RLM3_Random_Init();
	ASSERT(RLM3_Random_IsInit());
	RLM3_Random_Deinit();
	ASSERT(!RLM3_Random_IsInit());
}

TEST_CASE(Random_Get)
{
	static const size_t BUFFER_SIZE = 1024;
	uint8_t* buffer = new uint8_t[BUFFER_SIZE];
	size_t* counts = new size_t[256];

	RLM3_Random_Init();
	for (size_t i = 0; i < 100; i++)
	{
		RLM3_Random_Get(buffer, BUFFER_SIZE);
		for (size_t j = 0; j < BUFFER_SIZE; j++)
			counts[buffer[j]]++;
	}
	RLM3_Random_Deinit();

	size_t min = *std::min_element(counts, counts + 256);
	size_t max = *std::max_element(counts, counts + 256);

	LOG_ALWAYS("min: %d max: %d", min, max);

	delete[] buffer;
	delete[] counts;

	ASSERT(min >= 300 && max <= 500);
}
