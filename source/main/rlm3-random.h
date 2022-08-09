#pragma once

#include "rlm3-base.h"


#ifdef __cplusplus
extern "C" {
#endif


extern void RLM3_Random_Init();
extern void RLM3_Random_Deinit();
extern bool RLM3_Random_IsInit();

extern void RLM3_Random_Get(uint8_t* data, size_t size);


#ifdef __cplusplus
}
#endif
