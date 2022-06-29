#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct SpinLock
{
	volatile bool is_locked;
#ifdef TEST
	volatile void* owner;
#endif
} SpinLock;

extern void RLM3_SpinLock_Init(SpinLock* lock);
extern void RLM3_SpinLock_Deinit(SpinLock* lock);
extern void RLM3_SpinLock_Enter(SpinLock* lock);
extern bool RLM3_SpinLock_Try(SpinLock* lock, size_t timeout_ms);
extern void RLM3_SpinLock_Leave(SpinLock* lock);


typedef struct MutexLock
{
	volatile void* queue;
#ifdef TEST
	volatile void* owner;
#endif
} MutexLock;

extern void RLM3_MutexLock_Init(MutexLock* lock);
extern void RLM3_MutexLock_Deinit(MutexLock* lock);
extern void RLM3_MutexLock_Enter(MutexLock* lock);
extern bool RLM3_MutexLock_Try(MutexLock* lock, size_t timeout_ms);
extern void RLM3_MutexLock_Leave(MutexLock* lock);


#ifdef __cplusplus
}
#endif
