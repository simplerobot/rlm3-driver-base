#include "rlm3-i2c.h"
#include "rlm3-lock.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include "main.h"
#include "logger.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Assert.h"


LOGGER_ZONE(I2C);


enum
{
	I2C_STATE_IDLE,
	I2C_STATE_ERROR,
	I2C_STATE_TX_WAIT,
	I2C_STATE_TX_DONE,
	I2C_STATE_RX_WAIT,
	I2C_STATE_RX_DONE,
};


static SpinLock g_lock_i2c1;

static uint8_t g_active_devices_i2c1 = 0;

static volatile uint8_t g_state_i2c1 = I2C_STATE_IDLE;
static volatile TaskHandle_t g_waiting_thread_i2c1 = NULL;


static __attribute__((constructor)) void Init_I2C()
{
	RLM3_SpinLock_Init(&g_lock_i2c1);
}


extern void RLM3_I2C1_Init(RLM3_I2C1_DEVICE device)
{
	LOG_TRACE("I2C1 Init %d", device);

	ASSERT(device < RLM3_I2C1_DEVICE_COUNT);
	ASSERT(READ_BIT(g_active_devices_i2c1, 1 << device) == 0);

	RLM3_SpinLock_Enter(&g_lock_i2c1);
	if (g_active_devices_i2c1 == 0)
		MX_I2C1_Init();
	SET_BIT(g_active_devices_i2c1, 1 << device);
	RLM3_SpinLock_Leave(&g_lock_i2c1);
}

extern void RLM3_I2C1_Deinit(RLM3_I2C1_DEVICE device)
{
	LOG_TRACE("I2C1 Deinit %d", device);

	ASSERT(device < RLM3_I2C1_DEVICE_COUNT);
	ASSERT(READ_BIT(g_active_devices_i2c1, 1 << device) != 0);

	RLM3_SpinLock_Enter(&g_lock_i2c1);
	CLEAR_BIT(g_active_devices_i2c1, 1 << device);
	if (g_active_devices_i2c1 == 0)
		HAL_I2C_DeInit(&hi2c1);
	RLM3_SpinLock_Leave(&g_lock_i2c1);
}

extern bool RLM3_I2C1_IsInit(RLM3_I2C1_DEVICE device)
{
	return (READ_BIT(g_active_devices_i2c1, 1 << device) != 0);
}

extern bool RLM3_I2C1_Transmit(uint32_t addr, const uint8_t* data, size_t size)
{
	LOG_TRACE("I2C1 TX(%x) %d", (int)addr, size);

	ASSERT(g_active_devices_i2c1 != 0);
	ASSERT(addr <= 0x7F);
	ASSERT(data != NULL);
	ASSERT(size > 0);

	RLM3_SpinLock_Enter(&g_lock_i2c1);
	ASSERT(g_waiting_thread_i2c1 == NULL);
	ASSERT(g_state_i2c1 == I2C_STATE_IDLE);
	g_waiting_thread_i2c1 = xTaskGetCurrentTaskHandle();
	g_state_i2c1 = I2C_STATE_TX_WAIT;

	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_IT(&hi2c1, (addr << 1) | 0x00, (uint8_t*)data, size);
	while (status == HAL_OK && g_state_i2c1 == I2C_STATE_TX_WAIT)
	{
		LOG_TRACE("TAKE %d %d %d", status, hi2c1.State, g_state_i2c1);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		LOG_TRACE("DONE %d %d %d", status, hi2c1.State, g_state_i2c1);
	}
	bool result = (status == HAL_OK && g_state_i2c1 == I2C_STATE_TX_DONE);

	g_waiting_thread_i2c1 = NULL;
	g_state_i2c1 = I2C_STATE_IDLE;
	RLM3_SpinLock_Leave(&g_lock_i2c1);

	return result;
}

extern bool RLM3_I2C1_Receive(uint32_t addr, uint8_t* data, size_t size)
{
	LOG_TRACE("I2C1 RX(%x) %d", (int)addr, size);

	ASSERT(g_active_devices_i2c1 != 0);
	ASSERT(addr <= 0x7F);
	ASSERT(data != NULL);
	ASSERT(size > 0);

	RLM3_SpinLock_Enter(&g_lock_i2c1);
	ASSERT(g_waiting_thread_i2c1 == NULL);
	ASSERT(g_state_i2c1 == I2C_STATE_IDLE);
	g_waiting_thread_i2c1 = xTaskGetCurrentTaskHandle();
	g_state_i2c1 = I2C_STATE_RX_WAIT;

	HAL_StatusTypeDef status = HAL_I2C_Master_Receive_IT(&hi2c1, (addr << 1) | 0x01, data, size);
	while (status == HAL_OK && hi2c1.State == HAL_I2C_STATE_BUSY_RX)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	bool result = (status == HAL_OK && g_state_i2c1 == I2C_STATE_RX_DONE);

	g_waiting_thread_i2c1 = NULL;
	g_state_i2c1 = I2C_STATE_IDLE;
	RLM3_SpinLock_Leave(&g_lock_i2c1);

	return result;
}

extern bool RLM3_I2C1_TransmitReceive(uint32_t addr, const uint8_t* tx_data, size_t tx_size, uint8_t* rx_data, size_t rx_size)
{
	LOG_TRACE("I2C1 TR(%x) %d %d", (int)addr, tx_size, rx_size);

	ASSERT(g_active_devices_i2c1 != 0);
	ASSERT(addr <= 0x7F);
	ASSERT(tx_data != NULL && rx_data != NULL);
	ASSERT(tx_size > 0 && rx_size > 0);

	RLM3_SpinLock_Enter(&g_lock_i2c1);
	ASSERT(g_waiting_thread_i2c1 == NULL);
	ASSERT(g_state_i2c1 == I2C_STATE_IDLE);
	g_waiting_thread_i2c1 = xTaskGetCurrentTaskHandle();

	HAL_StatusTypeDef status = HAL_OK;

	if (status == HAL_OK)
	{
		g_state_i2c1 = I2C_STATE_TX_WAIT;
		status = HAL_I2C_Master_Seq_Transmit_IT(&hi2c1, (addr << 1) | 0x00, (uint8_t*)tx_data, tx_size, I2C_FIRST_FRAME);
	}
	while (status == HAL_OK && g_state_i2c1 == I2C_STATE_TX_WAIT)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (status == HAL_OK && g_state_i2c1 == I2C_STATE_TX_DONE)
	{
		g_state_i2c1 = I2C_STATE_RX_WAIT;
		status = HAL_I2C_Master_Seq_Receive_IT(&hi2c1, (addr << 1) | 0x01, rx_data, rx_size, I2C_LAST_FRAME);
	}
	while (status == HAL_OK && g_state_i2c1 == I2C_STATE_RX_WAIT)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	bool result = (status == HAL_OK && g_state_i2c1 == I2C_STATE_RX_DONE);

	g_waiting_thread_i2c1 = NULL;
	g_state_i2c1 = I2C_STATE_IDLE;
	RLM3_SpinLock_Leave(&g_lock_i2c1);

	return result;
}

static void WakeupWaitingThreadFromISR(I2C_HandleTypeDef *hi2c, uint8_t new_state)
{
	TaskHandle_t task = NULL;
	if (hi2c == &hi2c1)
	{
		task = g_waiting_thread_i2c1;
		g_state_i2c1 = new_state;
	}

	BaseType_t higher_priority_task_woken = pdFALSE;
	if (task != NULL)
		vTaskNotifyGiveFromISR(task, &higher_priority_task_woken);
	portYIELD_FROM_ISR(higher_priority_task_woken);
}

extern void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	LOG_TRACE("I2C INT TX");
	WakeupWaitingThreadFromISR(hi2c, I2C_STATE_TX_DONE);
}

extern void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	LOG_TRACE("I2C INT RX");
	WakeupWaitingThreadFromISR(hi2c, I2C_STATE_RX_DONE);
}

extern void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	LOG_TRACE("I2C INT ER");
	WakeupWaitingThreadFromISR(hi2c, I2C_STATE_ERROR);
}
