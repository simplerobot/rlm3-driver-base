#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
	RLM3_I2C1_DEVICE_TEST,
	RLM3_I2C1_DEVICE_CAMERA,
	RLM3_I2C1_DEVICE_MAGNETIC,
	RLM3_I2C1_DEVICE_FLASH,
	RLM3_I2C1_DEVICE_COUNT
} RLM3_I2C1_DEVICE;


extern void RLM3_I2C1_Init(RLM3_I2C1_DEVICE device);
extern void RLM3_I2C1_Deinit(RLM3_I2C1_DEVICE device);
extern bool RLM3_I2C1_IsInit(RLM3_I2C1_DEVICE device);

extern bool RLM3_I2C1_Transmit(uint32_t addr, const uint8_t* data, size_t size);
extern bool RLM3_I2C1_Receive(uint32_t addr, uint8_t* data, size_t size);
extern bool RLM3_I2C1_TransmitReceive(uint32_t addr, const uint8_t* tx_data, size_t tx_size, uint8_t* rx_data, size_t rx_size);


#ifdef __cplusplus
}
#endif
