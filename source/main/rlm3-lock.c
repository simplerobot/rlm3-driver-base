#include "rlm3-lock.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Assert.h"
#include "rlm3-atomic.h"


extern void RLM3_SpinLock_Init(SpinLock* lock)
{
	lock->is_locked = false;
#ifdef TEST
	lock->owner = NULL;
#endif
}

extern void RLM3_SpinLock_Deinit(SpinLock* lock)
{
	ASSERT(!lock->is_locked);
	ASSERT(lock->owner == NULL);
}

extern void RLM3_SpinLock_Enter(SpinLock* lock)
{
	ASSERT(!RLM3_IsIRQ());
	ASSERT(RLM3_IsSchedulerRunning());

	while (RLM3_Atomic_SetBool(&lock->is_locked))
	{
		taskYIELD();
		if (lock->is_locked)
			vTaskDelay(1);
	}

#ifdef TEST
	ASSERT(lock->owner == NULL);
	lock->owner = xTaskGetCurrentTaskHandle();
#endif
}

extern bool RLM3_SpinLock_Try(SpinLock* lock, size_t timeout_ms)
{
	ASSERT(!RLM3_IsIRQ());
	ASSERT(RLM3_IsSchedulerRunning());

	TickType_t start_time = xTaskGetTickCount();
	TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
	while (RLM3_Atomic_SetBool(&lock->is_locked))
	{
		if (xTaskGetTickCount() - start_time >= timeout_ticks)
			return false;

		taskYIELD();
		if (lock->is_locked)
			vTaskDelay(1);
	}

#ifdef TEST
	ASSERT(lock->owner == NULL);
	lock->owner = xTaskGetCurrentTaskHandle();
#endif

	return true;
}

extern void RLM3_SpinLock_Leave(SpinLock* lock)
{
	ASSERT(lock->is_locked);
	ASSERT(lock->owner == xTaskGetCurrentTaskHandle());
#ifdef TEST
	lock->owner = NULL;
#endif
	lock->is_locked = false;
}


#if configNUM_THREAD_LOCAL_STORAGE_POINTERS != 1
#error "Mutex Locks require a thread local storage element to maintain the waiting list"
#endif


static void SetNext(TaskHandle_t task, TaskHandle_t next)
{
	vTaskSetThreadLocalStoragePointer(task, 0, next);
}

static TaskHandle_t GetNext(TaskHandle_t task)
{
	return (TaskHandle_t)pvTaskGetThreadLocalStoragePointer(task, 0);
}

static void AtomicAddTaskToWaitQueue(MutexLock* lock, TaskHandle_t task)
{
	taskENTER_CRITICAL();
	vTaskSetThreadLocalStoragePointer(task, 0, (void*)lock->queue);
	lock->queue = task;
	taskEXIT_CRITICAL();
}

static void AtomicRemoveTaskFromWaitQueue(MutexLock* lock, TaskHandle_t task, TaskHandle_t* prev_out, TaskHandle_t* next_out)
{
	// Walk down the linked list to find the target task.  This assumes a short queue since this must not disable interrupts for long.
	TaskHandle_t prev = NULL;
	taskENTER_CRITICAL();
	TaskHandle_t cursor = (TaskHandle_t)lock->queue;
	TaskHandle_t next = GetNext(cursor);
	while (cursor != NULL && cursor != task)
	{
		prev = cursor;
		cursor = next;
		next = GetNext(cursor);
	}
	if (cursor != NULL)
	{
		if (prev != NULL)
			SetNext(prev, next);
		else
			lock->queue = next;
	}
	taskEXIT_CRITICAL();

	ASSERT(cursor != NULL);

	SetNext(task, NULL);

	if (prev_out != NULL)
		*prev_out = prev;
	if (next_out != NULL)
		*next_out = next;
}


extern void RLM3_MutexLock_Init(MutexLock* lock)
{
	lock->queue = NULL;
#ifdef TEST
	lock->owner = NULL;
#endif
}

extern void RLM3_MutexLock_Deinit(MutexLock* lock)
{
	ASSERT(lock->queue == NULL);
	ASSERT(lock->owner == NULL);
}

extern void RLM3_MutexLock_Enter(MutexLock* lock)
{
	ASSERT(!RLM3_IsIRQ());
	ASSERT(RLM3_IsSchedulerRunning());

	// Make sure this is a valid active task.
	TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
	TaskHandle_t blocking_task = GetNext(current_task);
	ASSERT(blocking_task == NULL);
	ASSERT(current_task != lock->owner);

	AtomicAddTaskToWaitQueue(lock, current_task);

	// Wait until no other tasks are in front of us.
	while ((blocking_task = GetNext(current_task)) != NULL)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

#ifdef TEST
	ASSERT(lock->owner == NULL);
	lock->owner = (void*)current_task;
#endif
}

extern bool RLM3_MutexLock_Try(MutexLock* lock, size_t timeout_ms)
{
	ASSERT(!RLM3_IsIRQ());
	ASSERT(RLM3_IsSchedulerRunning());

	// Make sure this is a valid active task.
	TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
	TaskHandle_t blocking_task = GetNext(current_task);
	ASSERT(blocking_task == NULL);
	ASSERT(current_task != lock->owner);

	AtomicAddTaskToWaitQueue(lock, current_task);

	// Wait until no other tasks are in front of us.
	TickType_t start_time = xTaskGetTickCount();
	TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
	while ((blocking_task = GetNext(current_task)) != NULL)
	{
		// Check for timeout.
		TickType_t current_time = xTaskGetTickCount();
		if (current_time - start_time >= timeout_ticks)
		{
			// We timed out.  Remove our task from the waiting queue.
			TaskHandle_t prev = NULL;
			TaskHandle_t next = NULL;
			AtomicRemoveTaskFromWaitQueue(lock, current_task, &prev, &next);

			// If we actually may have gotten a notification, pass it on to the new candidate.
			if (next == NULL && prev != NULL)
				xTaskNotifyGive(prev);
			return false;
		}

		// Wait for notification.
		TickType_t wait_time = start_time + timeout_ticks - current_time;
		ulTaskNotifyTake(pdTRUE, wait_time);
	}

#ifdef TEST
	ASSERT(lock->owner == NULL);
	lock->owner = (void*)current_task;
#endif

	return true;
}

extern void RLM3_MutexLock_Leave(MutexLock* lock)
{
	ASSERT(!RLM3_IsIRQ());
	ASSERT(RLM3_IsSchedulerRunning());

	// Make sure this is a valid task that owns this lock.
	TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
	TaskHandle_t blocking_task = GetNext(current_task);
	ASSERT(blocking_task == NULL);
	ASSERT(lock->queue != NULL);
	ASSERT(lock->owner == current_task);

	// Remove this task from the lock's queue.
	TaskHandle_t blocked_task = NULL;
#ifdef TEST
	lock->owner = NULL;
#endif
	AtomicRemoveTaskFromWaitQueue(lock, current_task, &blocked_task, NULL);

	// Wake up the next task in the queue.
	if (blocked_task != NULL)
		xTaskNotifyGive(blocked_task);
}

