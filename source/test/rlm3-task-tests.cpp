#include "Test.hpp"
#include "rlm3-task.h"
#include "main.h"
#include <algorithm>
#include "logger.h"
#include "cmsis_os2.h"
#include "rlm3-timer.h"


LOGGER_ZONE(TASK_TEST);


typedef void (*TimerFn)();
extern void SetTimer2Callback(TimerFn timer_fn);


TEST_CASE(Task_GetCurrentTime_HappyCase)
{
	RLM3_Time time0 = RLM3_GetCurrentTime();
	size_t steps_0 = 0;
	while (steps_0 < 1000000000 && RLM3_GetCurrentTime() == time0)
		steps_0++;
	RLM3_Time time1 = RLM3_GetCurrentTime();
	size_t steps_1 = 0;
	while (steps_1 < 1000000000 && RLM3_GetCurrentTime() == time1)
		steps_1++;
	RLM3_Time time2 = RLM3_GetCurrentTime();
	size_t steps_2 = 0;
	while (steps_2 < 1000000000 && RLM3_GetCurrentTime() == time2)
		steps_2++;

	ASSERT(time1 == time0 + 1);
	ASSERT(time2 == time1 + 1);
	size_t steps_max = std::max(steps_1, steps_2);
	size_t steps_min = std::min(steps_1, steps_2);
	ASSERT(1.0 * steps_min / steps_max > 0.99);
}

TEST_CASE(Task_Yield_SingleThread)
{
	RLM3_Time start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < 10; i++)
		RLM3_Yield();
	RLM3_Time end_time = RLM3_GetCurrentTime();

	ASSERT((end_time - start_time) <= 1);
}

TEST_CASE(Task_Yield_HappyCase)
{
	auto secondary_thread_fn = [](void* param)
	{
		for (size_t i = 0; i < 10; i++)
		{
			(*(size_t*)param)++;
			RLM3_Yield();
		}
		::osThreadExit();
	};

	size_t secondary_count = 0;
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	ASSERT(::osThreadNew(secondary_thread_fn, &secondary_count, &task_attributes) != nullptr);

	RLM3_Time start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < 10; i++)
	{
		ASSERT(secondary_count == i);
		RLM3_Yield();
	}
	RLM3_Time end_time = RLM3_GetCurrentTime();

	ASSERT((end_time - start_time) <= 1);
}

TEST_CASE(Task_Delay_HappyCase)
{
	RLM3_Time time0 = RLM3_GetCurrentTime();
	RLM3_Delay(1);
	RLM3_Time time1 = RLM3_GetCurrentTime();
	RLM3_Delay(1);
	RLM3_Time time2 = RLM3_GetCurrentTime();
	RLM3_Delay(1);
	RLM3_Time time3 = RLM3_GetCurrentTime();

	ASSERT(time0 + 1 <= time1 && time1 <= time0 + 3);
	ASSERT(time1 + 1 <= time2 && time2 <= time1 + 2);
	ASSERT(time2 + 1 <= time3 && time3 <= time2 + 2);
}

TEST_CASE(Task_DelayUntil_HappyCase)
{
	RLM3_Time start_time = RLM3_GetCurrentTime();
	RLM3_DelayUntil(start_time, 3);
	RLM3_Time actual_time = RLM3_GetCurrentTime();

	ASSERT(start_time + 3 == actual_time);
}

TEST_CASE(Task_GetCurrentTask_SingleThread)
{
	RLM3_Task task = RLM3_GetCurrentTask();
	ASSERT(task != nullptr);
	ASSERT(task == RLM3_GetCurrentTask());
}

TEST_CASE(Task_GetCurrentTask_MultipleThreads)
{
	auto secondary_thread_fn = [](void* param)
	{
		*(RLM3_Task*)param = RLM3_GetCurrentTask();
		::osThreadExit();
	};

	RLM3_Task secondary_tasks[6] = {};
	osThreadId_t secondary_ids[6] = {};
	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	for (size_t i = 0; i < 6; i++)
		secondary_ids[i] = ::osThreadNew(secondary_thread_fn, &secondary_tasks[i], &task_attributes);
	RLM3_Delay(1);

	RLM3_Task current_task = RLM3_GetCurrentTask();
	for (size_t i = 0; i < 6; i++)
	{
		ASSERT(secondary_ids[i] != nullptr);
		ASSERT(secondary_tasks[i] != nullptr);
		ASSERT(secondary_ids[i] == secondary_tasks[i]);
		ASSERT(secondary_tasks[i] != current_task);
		for (size_t j = i + 1; j < 6; j++)
			ASSERT(secondary_tasks[i] != secondary_tasks[j]);
	}
}

TEST_CASE(Task_Give_HappyCase)
{
	RLM3_Give(RLM3_GetCurrentTask());

	RLM3_Take();
}

TEST_CASE(Task_Give_SecondThread)
{
	auto secondary_thread_fn = [](void* param)
	{
		for (size_t i = 0; i < 5; i++)
		{
			RLM3_Take();
			(*(size_t*)param)++;
		}
		::osThreadExit();
	};

	size_t secondary_count = 0;

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	RLM3_Task secondary_task = ::osThreadNew(secondary_thread_fn, &secondary_count, &task_attributes);
	ASSERT(secondary_task != nullptr);

	RLM3_Time start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < 5; i++)
	{
		RLM3_Yield();
		ASSERT(secondary_count == i);
		RLM3_Give(secondary_task);
		RLM3_Yield();
	}
	RLM3_Time end_time = RLM3_GetCurrentTime();

	ASSERT((end_time - start_time) <= 1);
}

TEST_CASE(Task_Give_HigherPriority)
{
	auto secondary_thread_fn = [](void* param)
	{
		for (size_t i = 0; i < 5; i++)
		{
			RLM3_Take();
			(*(size_t*)param)++;
		}
		::osThreadExit();
	};

	size_t secondary_count = 0;

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityAboveNormal;
	RLM3_Task secondary_task = ::osThreadNew(secondary_thread_fn, &secondary_count, &task_attributes);
	ASSERT(secondary_task != nullptr);

	RLM3_Time start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < 5; i++)
	{
		ASSERT(secondary_count == i);
		RLM3_Give(secondary_task);
	}
	RLM3_Time end_time = RLM3_GetCurrentTime();

	ASSERT((end_time - start_time) <= 1);
}

TEST_CASE(Task_Give_Multiple)
{
	auto secondary_thread_fn = [](void* param)
	{
		for (size_t i = 0; i < 2; i++)
		{
			RLM3_Take();
			(*(size_t*)param)++;
		}
		::osThreadExit();
	};

	size_t secondary_count = 0;

	osThreadAttr_t task_attributes = {};
	task_attributes.name = "secondary_thread";
	task_attributes.stack_size = 128 * 4;
	task_attributes.priority = osPriorityNormal;
	RLM3_Task secondary_task = ::osThreadNew(secondary_thread_fn, &secondary_count, &task_attributes);
	ASSERT(secondary_task != nullptr);

	RLM3_Delay(0); // Make sure we don't have a forced task switch between the next two lines.
	RLM3_Give(secondary_task);
	RLM3_Give(secondary_task);
	RLM3_Yield();
	RLM3_Yield();
	ASSERT(secondary_count == 1);
	RLM3_Give(secondary_task);
	RLM3_Yield();
	ASSERT(secondary_count == 2);
}

TEST_CASE(Task_Give_FromISR)
{
	static RLM3_Task g_target_task = RLM3_GetCurrentTask();
	SetTimer2Callback([] { RLM3_GiveFromISR(g_target_task); });
	RLM3_Timer2_Init(5000);

	RLM3_Time start_time = RLM3_GetCurrentTime();
	for (size_t i = 0; i < 100; i++)
		RLM3_Take();
	RLM3_Time end_time = RLM3_GetCurrentTime();

	RLM3_Timer2_Deinit();

	RLM3_Time elapsed = end_time - start_time;
	ASSERT(20 <= elapsed && elapsed <= 21);
}

TEST_CASE(Task_TakeWithTimeout_HappyCase)
{
	RLM3_Give(RLM3_GetCurrentTask());

	RLM3_Time start_time = RLM3_GetCurrentTime();
	bool result = RLM3_TakeWithTimeout(10);
	RLM3_Time elapsed = RLM3_GetCurrentTime() - start_time;

	ASSERT(result);
	ASSERT(elapsed <= 1);
}

TEST_CASE(Task_TakeWithTimeout_Timeout)
{
	RLM3_Time start_time = RLM3_GetCurrentTime();
	bool result = RLM3_TakeWithTimeout(10);
	RLM3_Time elapsed = RLM3_GetCurrentTime() - start_time;

	ASSERT(!result);
	ASSERT(10 <= elapsed && elapsed <= 11);
}

TEST_CASE(Task_TakeUntil_HappyCase)
{
	RLM3_Give(RLM3_GetCurrentTask());
	RLM3_Time start_time = RLM3_GetCurrentTime();

	bool result = RLM3_TakeUntil(start_time, 10);
	RLM3_Time elapsed = RLM3_GetCurrentTime() - start_time;

	ASSERT(result);
	ASSERT(elapsed <= 1);
}

TEST_CASE(Task_TakeUntil_Timeout)
{
	RLM3_Time start_time = RLM3_GetCurrentTime();
	RLM3_Time target_time = start_time + 10;

	bool result = RLM3_TakeUntil(start_time, 10);
	RLM3_Time end_time = RLM3_GetCurrentTime();

	ASSERT(!result);
	ASSERT(end_time == target_time);
}

TEST_CASE(Task_EnterCritical)
{
	static volatile uint32_t g_count = 0;
	SetTimer2Callback([] { ASSERT(g_count % 1000 == 0); });
	RLM3_Timer2_Init(5000);

	for (size_t i = 0; i < 1000; i++)
	{
		RLM3_EnterCritical();
		for (size_t j = 0; j < 1000; j++)
			g_count++;
		RLM3_ExitCritical();
	}

	RLM3_Timer2_Deinit();
}

TEST_CASE(Task_EnterCriticalFromISR)
{
	SetTimer2Callback([] {
		uint32_t saved = RLM3_EnterCriticalFromISR();
		RLM3_ExitCriticalFromISR(saved);
	});
	RLM3_Timer2_Init(5000);
	RLM3_Delay(10);
	RLM3_Timer2_Deinit();
}
