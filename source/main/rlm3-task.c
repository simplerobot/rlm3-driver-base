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
	ASSERT(!IsISR());
	return xTaskGetTickCount();
}

extern RLM3_Time RLM3_GetCurrentTimeFromISR()
{
	ASSERT(IsISR());
	return xTaskGetTickCountFromISR();
}

extern void RLM3_Yield()
{
	ASSERT(!IsISR());
	taskYIELD();
}

extern void RLM3_Delay(RLM3_Time delay_ms)
{
	ASSERT(!IsISR());
	vTaskDelay(delay_ms + 1);
}

extern void RLM3_DelayUntil(RLM3_Time start_time, RLM3_Time delay_ms)
{
	ASSERT(!IsISR());
	RLM3_Time start_time_copy = start_time;
	vTaskDelayUntil(&start_time_copy, delay_ms);
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

extern bool RLM3_TakeWithTimeout(RLM3_Time timeout_ms)
{
	ASSERT(!IsISR());
	uint32_t value = ulTaskNotifyTake(pdTRUE, timeout_ms + 1);
	return (value > 0);
}

extern bool RLM3_TakeUntil(RLM3_Time start_time, RLM3_Time delay_ms)
{
	ASSERT(!IsISR());
	RLM3_Time current_time = xTaskGetTickCount();
	if (current_time - start_time >= delay_ms)
		return false;
	uint32_t value = ulTaskNotifyTake(pdTRUE, delay_ms + start_time - current_time);
	return (value > 0);
}

extern void RLM3_EnterCritical()
{
	taskENTER_CRITICAL();
}

extern uint32_t RLM3_EnterCriticalFromISR()
{
	return taskENTER_CRITICAL_FROM_ISR();
}

extern void RLM3_ExitCritical()
{
	taskEXIT_CRITICAL();
}

extern void RLM3_ExitCriticalFromISR(uint32_t saved_level)
{
	taskEXIT_CRITICAL_FROM_ISR(saved_level);
}
