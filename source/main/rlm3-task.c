#include "rlm3-task.h"
#include <stm32f4xx.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Assert.h"


static bool IsISR()
{
	return (__get_IPSR() != 0U);
}

extern RLM3_Time RLM3_GetCurrentTime()
{
	if (IsISR())
		return xTaskGetTickCountFromISR();
	else
		return xTaskGetTickCount();
}

extern void RLM3_Yield()
{
	ASSERT(!IsISR());
	taskYIELD();
}

extern void RLM3_Delay(RLM3_Time time_ms)
{
	ASSERT(!IsISR());
	vTaskDelay(time_ms + 1);
}

extern void RLM3_DelayUntil(RLM3_Time target_time)
{
	ASSERT(!IsISR());
	RLM3_Time current_time = xTaskGetTickCount();
	RLM3_Time delay = target_time - current_time;
	if ((int)delay > 0)
		vTaskDelayUntil(&current_time, delay);
}

extern RLM3_Task RLM3_GetCurrentTask()
{
	return xTaskGetCurrentTaskHandle();
}

extern void RLM3_Give(RLM3_Task task)
{
	ASSERT(!IsISR());
	if (task != NULL)
	{
		xTaskNotifyGive(task);
	}
}

extern void RLM3_GiveFromISR(RLM3_Task task)
{
	ASSERT(IsISR());
	if (task != NULL)
	{
		BaseType_t higher_priority_task_woken = pdFALSE;
		vTaskNotifyGiveFromISR(task, &higher_priority_task_woken);
		portYIELD_FROM_ISR(higher_priority_task_woken);
	}
}

extern void RLM3_Take()
{
	ASSERT(!IsISR());
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

extern bool RLM3_TakeTimeout(RLM3_Time timeout_ms)
{
	ASSERT(!IsISR());
	uint32_t value = ulTaskNotifyTake(pdTRUE, timeout_ms + 1);
	return (value > 0);
}

extern bool RLM3_TakeUntil(RLM3_Time target_time)
{
	ASSERT(!IsISR());
	RLM3_Time current_time = xTaskGetTickCount();
	RLM3_Time delay = target_time - current_time;
	if ((int)delay <= 0)
		return false;
	uint32_t value = ulTaskNotifyTake(pdTRUE, delay);
	return (value > 0);
}


