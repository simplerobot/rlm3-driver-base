#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RLM3_EXTERNAL_MEMORY_ADDRESS ((uint8_t*)0xD0000000)
#define RLM3_EXTERNAL_MEMORY_SIZE (8 * 1024 * 1024)


extern void RLM3_MEMORY_Init();
extern void RLM3_MEMORY_Deinit();
extern bool RLM3_MEMORY_IsInit();


#ifdef __cplusplus
}
#endif


