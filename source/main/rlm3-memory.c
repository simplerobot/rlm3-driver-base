#include "rlm3-memory.h"
#include "fmc.h"
#include "main.h"
#include "rlm3-task.h"


static bool g_is_initialized = false;


extern void RLM3_MEMORY_Init()
{
	MX_FMC_Init();
	g_is_initialized = true;
	RLM3_Delay(1000);
}

extern void RLM3_MEMORY_Deinit()
{
	g_is_initialized = false;
	HAL_SDRAM_DeInit(&hsdram2);
}

extern bool RLM3_MEMORY_IsInit()
{
	return g_is_initialized;
}

void HAL_SDRAM_RefreshErrorCallback(SDRAM_HandleTypeDef *hsdram)
{

}
