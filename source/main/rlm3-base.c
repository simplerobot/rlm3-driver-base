#include "rlm3-base.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"


extern bool RLM3_IsIRQ()
{
	return (__get_IPSR() != 0U);
}

extern bool RLM3_IsSchedulerRunning()
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

extern void RLM3_DebugOutput(uint8_t c)
{
	if ((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0)
		return;
	if ((ITM->TER & 1UL) == 0)
		return;
	while (ITM->PORT[0U].u32 == 0UL)
		taskYIELD();
	ITM->PORT[0U].u8 = c;
}

extern bool RLM3_DebugOutputFromISR(uint8_t c)
{
	if ((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0)
		return false;
	if ((ITM->TER & 1UL) == 0)
		return false;
	if (ITM->PORT[0U].u32 == 0UL)
		return false;
	ITM->PORT[0U].u8 = c;
	return true;
}
