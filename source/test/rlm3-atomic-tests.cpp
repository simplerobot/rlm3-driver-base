#include "Test.hpp"
#include "rlm3-atomic.h"
#include "cmsis_os2.h"


TEST_CASE(Atomic_SetBool_Value)
{
	bool value = false;

	ASSERT(RLM3_Atomic_SetBool(&value) == false);
	ASSERT(RLM3_Atomic_SetBool(&value) == true);
}

TEST_CASE(Atomic_Inc8_Value)
{
	uint8_t value = 0;

	ASSERT(RLM3_Atomic_Inc8(&value) == 1);
	ASSERT(RLM3_Atomic_Inc8(&value) == 2);
	value = 0xFE;
	ASSERT(RLM3_Atomic_Inc8(&value) == 0xFF);
	ASSERT(RLM3_Atomic_Inc8(&value) == 0);
}

TEST_CASE(Atomic_Dec8_Value)
{
	uint8_t value = 2;

	ASSERT(RLM3_Atomic_Dec8(&value) == 1);
	ASSERT(RLM3_Atomic_Dec8(&value) == 0);
	ASSERT(RLM3_Atomic_Dec8(&value) == 0xFF);
	ASSERT(RLM3_Atomic_Dec8(&value) == 0xFE);
}

TEST_CASE(Atomic_Inc8_Threads)
{
	static uint8_t value = 0;

	auto secondary_thread_fn = [](void*)
	{
		for (size_t i = 0; i < 32; i++)
		{
			RLM3_Atomic_Inc8(&value);
			::osThreadYield();
		}
		::osThreadExit();
	};

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	for (size_t i = 0; i < 4; i++)
		ASSERT(::osThreadNew(secondary_thread_fn, NULL, &task_attributes) != nullptr);

	uint32_t start_time = osKernelGetTickCount();
	while (osKernelGetTickCount() - start_time < 5 && value < 128)
		::osThreadYield();

	ASSERT(value == 128);
}

TEST_CASE(Atomic_Inc16_Value)
{
	uint16_t value = 0;

	ASSERT(RLM3_Atomic_Inc16(&value) == 1);
	ASSERT(RLM3_Atomic_Inc16(&value) == 2);
	value = 0xFFFE;
	ASSERT(RLM3_Atomic_Inc16(&value) == 0xFFFF);
	ASSERT(RLM3_Atomic_Inc16(&value) == 0);
}

TEST_CASE(Atomic_Dec16_Value)
{
	uint16_t value = 2;

	ASSERT(RLM3_Atomic_Dec16(&value) == 1);
	ASSERT(RLM3_Atomic_Dec16(&value) == 0);
	ASSERT(RLM3_Atomic_Dec16(&value) == 0xFFFF);
	ASSERT(RLM3_Atomic_Dec16(&value) == 0xFFFE);
}


TEST_CASE(Atomic_Inc32_Value)
{
	uint32_t value = 0;

	ASSERT(RLM3_Atomic_Inc32(&value) == 1);
	ASSERT(RLM3_Atomic_Inc32(&value) == 2);
	value = 0xFFFFFFFE;
	ASSERT(RLM3_Atomic_Inc32(&value) == 0xFFFFFFFF);
	ASSERT(RLM3_Atomic_Inc32(&value) == 0);
}

TEST_CASE(Atomic_Dec32_Value)
{
	uint32_t value = 2;

	ASSERT(RLM3_Atomic_Dec32(&value) == 1);
	ASSERT(RLM3_Atomic_Dec32(&value) == 0);
	ASSERT(RLM3_Atomic_Dec32(&value) == 0xFFFFFFFF);
	ASSERT(RLM3_Atomic_Dec32(&value) == 0xFFFFFFFE);
}
