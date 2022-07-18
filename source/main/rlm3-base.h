#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


extern bool RLM3_IsIRQ();
extern bool RLM3_IsSchedulerRunning();
extern void RLM3_DebugOutput(uint8_t c);
extern bool RLM3_DebugOutputFromISR(uint8_t c);


#ifdef __cplusplus
}
#endif
