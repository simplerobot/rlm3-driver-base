#include "rlm3-gpio.h"


extern __attribute((weak)) void RLM3_EXTI12_Callback()
{
}

extern void EXTI15_10_IRQHandler()
{
	const uint32_t supported_interrupts = EXTI_PR_PR12;

	// Get all pending interrupts.
	uint32_t pending_interrupts = EXTI->PR;

	// Clear any pending interrupts that will be handled in this function.
	EXTI->PR = pending_interrupts & supported_interrupts;

	// Call the handlers for each handled interrupt.
	if ((pending_interrupts & EXTI_PR_PR12) != 0)
		RLM3_EXTI12_Callback();
}
