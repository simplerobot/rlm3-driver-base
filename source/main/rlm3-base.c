#include "rlm3-base.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stm32f4xx.h>


extern bool RLM3_IsIRQ()
{
	return (__get_IPSR() != 0U);
}

extern bool RLM3_IsSchedulerRunning()
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}
