#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


extern bool RLM3_Atomic_SetBool(volatile bool* value);
extern uint8_t RLM3_Atomic_Inc8(volatile uint8_t* value);
extern uint8_t RLM3_Atomic_Dec8(volatile uint8_t* value);
extern uint16_t RLM3_Atomic_Inc16(volatile uint16_t* value);
extern uint16_t RLM3_Atomic_Dec16(volatile uint16_t* value);
extern uint32_t RLM3_Atomic_Inc32(volatile uint32_t* value);
extern uint32_t RLM3_Atomic_Dec32(volatile uint32_t* value);


#ifdef __cplusplus
}
#endif
