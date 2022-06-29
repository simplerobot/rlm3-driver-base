#include "Test.hpp"
#include "rlm3-lock.h"
#include "cmsis_os2.h"
#include "logger.h"


LOGGER_ZONE(STRESS);


TEST_CASE(SpinLock_MultipleThreads_StressTest)
{
	static SpinLock g_lock;
	static volatile bool g_is_done = false;
	static volatile size_t g_total_count = 0;
	static volatile size_t g_miss_count = 0;

	auto enter_thread_fn = [](void* param)
	{
		volatile size_t* self_count = (size_t*)param;
		while (!g_is_done)
		{
			RLM3_SpinLock_Enter(&g_lock);
			(*self_count)++;
			g_total_count++;
			::osThreadYield();
			RLM3_SpinLock_Leave(&g_lock);
		}
		::osThreadExit();
	};

	auto try_thread_fn = [](void* param)
	{
		volatile size_t* self_count = (size_t*)param;
		size_t miss_count = 0;
		while (!g_is_done)
		{
			if (RLM3_SpinLock_Try(&g_lock, 1))
			{
				(*self_count)++;
				g_total_count++;
				g_miss_count += miss_count;
				miss_count = 0;
				::osThreadYield();
				RLM3_SpinLock_Leave(&g_lock);
			}
			else
				miss_count++;
		}
		::osThreadExit();
	};

	RLM3_SpinLock_Init(&g_lock);
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	size_t counts[8] = {};
	for (size_t i = 0; i < 4; i++)
		ASSERT(::osThreadNew(enter_thread_fn, counts + i, &task_attributes) != nullptr);
	for (size_t i = 4; i < 8; i++)
		ASSERT(::osThreadNew(try_thread_fn, counts + i, &task_attributes) != nullptr);

	uint32_t start_time = osKernelGetTickCount();
	for (size_t i = 1; i <= 30; i++)
	{
		::osDelayUntil(start_time + 1000 * i);
		LOG_ALWAYS("%d Total %d Miss %d Enter %d %d %d %d Try %d %d %d %d", i, g_total_count, g_miss_count, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7]);
	}
	g_is_done = true;
}

TEST_CASE(MutexLock_MultipleThreads_StressTest)
{
	static MutexLock g_lock;
	static volatile bool g_is_done = false;
	static volatile size_t g_total_count = 0;
	static volatile size_t g_miss_count = 0;

	auto enter_thread_fn = [](void* param)
	{
		volatile size_t* self_count = (size_t*)param;
		while (!g_is_done)
		{
			RLM3_MutexLock_Enter(&g_lock);
			(*self_count)++;
			g_total_count++;
			::osThreadYield();
			RLM3_MutexLock_Leave(&g_lock);
		}
		::osThreadExit();
	};

	auto try_thread_fn = [](void* param)
	{
		volatile size_t* self_count = (size_t*)param;
		size_t miss_count = 0;
		while (!g_is_done)
		{
			if (RLM3_MutexLock_Try(&g_lock, 1))
			{
				(*self_count)++;
				g_total_count++;
				g_miss_count += miss_count;
				miss_count = 0;
				::osThreadYield();
				RLM3_MutexLock_Leave(&g_lock);
			}
			else
				miss_count++;
		}
		::osThreadExit();
	};

	RLM3_MutexLock_Init(&g_lock);
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	size_t counts[8] = {};
	for (size_t i = 0; i < 4; i++)
		ASSERT(::osThreadNew(enter_thread_fn, counts + i, &task_attributes) != nullptr);
	for (size_t i = 4; i < 8; i++)
		ASSERT(::osThreadNew(try_thread_fn, counts + i, &task_attributes) != nullptr);

	uint32_t start_time = osKernelGetTickCount();
	for (size_t i = 1; i <= 30; i++)
	{
		::osDelayUntil(start_time + 1000 * i);
		LOG_ALWAYS("%d Total %d Miss %d Enter %d %d %d %d Try %d %d %d %d", i, g_total_count, g_miss_count, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7]);
	}
	g_is_done = true;
}

