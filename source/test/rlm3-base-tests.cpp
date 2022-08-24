#include "Test.hpp"
#include "rlm3-base.h"
#include "logger.h"


LOGGER_ZONE(BASE_TEST);


TEST_CASE(GetUniqueDeviceId_HappyCase)
{
	uint8_t id[12];
	RLM3_GetUniqueDeviceId(id);

	LOG_ALWAYS("Device ID: %x %x %x %x %x %x %x %x %x %x %x %x", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9], id[10], id[11]);
}

TEST_CASE(IsDebugOutput_HappyCase)
{
	ASSERT(RLM3_IsDebugOutput());
}
