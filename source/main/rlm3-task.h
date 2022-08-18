#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void* RLM3_Task;
typedef uint32_t RLM3_Time;

extern RLM3_Time RLM3_GetCurrentTime();
extern RLM3_Time RLM3_GetCurrentTimeFromISR();
extern void RLM3_Yield();
extern void RLM3_Delay(RLM3_Time time_ms);
extern void RLM3_DelayUntil(RLM3_Time start_time, RLM3_Time delay_ms);

extern RLM3_Task RLM3_GetCurrentTask();
extern void RLM3_Give(RLM3_Task task);
extern void RLM3_GiveFromISR(RLM3_Task task);
extern void RLM3_Take();
extern bool RLM3_TakeWithTimeout(RLM3_Time timeout_ms);
extern bool RLM3_TakeUntil(RLM3_Time start_time, RLM3_Time delay_ms);

extern void RLM3_EnterCritical();
extern uint32_t RLM3_EnterCriticalFromISR();
extern void RLM3_ExitCritical();
extern void RLM3_ExitCriticalFromISR(uint32_t saved_level);


#ifdef __cplusplus
}
#endif


