#pragma once

#include "rlm3-base.h"
#include "rlm3-task.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	volatile bool is_locked;
#ifdef TEST
	volatile RLM3_Task owner;
#endif
} RLM3_SpinLock;

extern void RLM3_SpinLock_Init(RLM3_SpinLock* lock);
extern void RLM3_SpinLock_Deinit(RLM3_SpinLock* lock);
extern void RLM3_SpinLock_Enter(RLM3_SpinLock* lock);
extern bool RLM3_SpinLock_Try(RLM3_SpinLock* lock, size_t timeout_ms);
extern void RLM3_SpinLock_Leave(RLM3_SpinLock* lock);


typedef struct
{
	volatile void* queue;
#ifdef TEST
	volatile RLM3_Task owner;
#endif
} RLM3_MutexLock;

extern void RLM3_MutexLock_Init(RLM3_MutexLock* lock);
extern void RLM3_MutexLock_Deinit(RLM3_MutexLock* lock);
extern void RLM3_MutexLock_Enter(RLM3_MutexLock* lock);
extern bool RLM3_MutexLock_Try(RLM3_MutexLock* lock, size_t timeout_ms);
extern void RLM3_MutexLock_Leave(RLM3_MutexLock* lock);


#ifdef __cplusplus
}
#endif
