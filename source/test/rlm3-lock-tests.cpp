#include "Test.hpp"
#include "rlm3-lock.h"
#include "cmsis_os2.h"


TEST_CASE(SpinLock_Lifecycle_HappyCase)
{
	SpinLock test;
	RLM3_SpinLock_Init(&test);

	RLM3_SpinLock_Enter(&test);
	RLM3_SpinLock_Leave(&test);

	RLM3_SpinLock_Deinit(&test);
}

TEST_CASE(SpinLock_MultipleThreads_SharedWork)
{
	static SpinLock test;
	auto secondary_thread_fn = [](void* param)
	{
		volatile size_t* value = (size_t*)param;
		RLM3_SpinLock_Enter(&test);
		size_t temp = *value;
		::osDelay(3);
		*value = (temp + 1);
		RLM3_SpinLock_Leave(&test);
		::osThreadExit();
	};

	RLM3_SpinLock_Init(&test);
	size_t count = 0;
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	for (size_t i = 0; i < 4; i++)
		ASSERT(::osThreadNew(secondary_thread_fn, &count, &task_attributes) != nullptr);

	::osDelay(15);
	ASSERT(count == 4);
}

TEST_CASE(SpinLock_Try_HappyCase)
{
	auto secondary_thread_fn = [](void* param)
	{
		SpinLock* lock = (SpinLock*)param;
		RLM3_SpinLock_Enter(lock);
		::osDelay(5);
		RLM3_SpinLock_Leave(lock);
		::osThreadExit();
	};

	SpinLock test;
	RLM3_SpinLock_Init(&test);

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	ASSERT(::osThreadNew(secondary_thread_fn, &test, &task_attributes) != nullptr);
	::osThreadYield();

	ASSERT(!RLM3_SpinLock_Try(&test, 3));
	ASSERT(RLM3_SpinLock_Try(&test, 3));
}

TEST_CASE(MutexLock_Lifecycle_HappyCase)
{
	MutexLock test;
	RLM3_MutexLock_Init(&test);

	RLM3_MutexLock_Enter(&test);
	RLM3_MutexLock_Leave(&test);

	RLM3_MutexLock_Deinit(&test);
}

TEST_CASE(MutexLock_MultipleThreads_SharedWork)
{
	static MutexLock test;
	auto secondary_thread_fn = [](void* param)
	{
		volatile size_t* value = (size_t*)param;
		RLM3_MutexLock_Enter(&test);
		size_t temp = *value;
		::osDelay(3);
		*value = (temp + 1);
		RLM3_MutexLock_Leave(&test);
		::osThreadExit();
	};

	RLM3_MutexLock_Init(&test);
	size_t count = 0;
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	for (size_t i = 0; i < 4; i++)
		ASSERT(::osThreadNew(secondary_thread_fn, &count, &task_attributes) != nullptr);

	::osDelay(15);
	ASSERT(count == 4);
}

TEST_CASE(MutexLock_Try_HappyCase)
{
	auto secondary_thread_fn = [](void* param)
	{
		MutexLock* lock = (MutexLock*)param;
		RLM3_MutexLock_Enter(lock);
		::osDelay(5);
		RLM3_MutexLock_Leave(lock);
		::osThreadExit();
	};

	MutexLock test;
	RLM3_MutexLock_Init(&test);

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	ASSERT(::osThreadNew(secondary_thread_fn, &test, &task_attributes) != nullptr);
	::osThreadYield();

	ASSERT(!RLM3_MutexLock_Try(&test, 3));
	ASSERT(RLM3_MutexLock_Try(&test, 3));
}


