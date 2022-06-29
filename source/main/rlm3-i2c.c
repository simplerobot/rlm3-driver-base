#include "rlm3-i2c.h"


extern void RLM3_I2C1_Init()
{
}

extern void RLM3_I2C1_Deinit()
{
}

extern void RLM3_I2C1_Start()
{
}

extern void RLM3_I2C1_Stop()
{
}

extern bool RLM3_I2C1_Transmit(I2cAddress addr, const uint8_t* data, size_t size)
{
	return false;
}

extern bool RLM3_I2C1_Receive(I2cAddress addr, uint8_t* data, size_t size)
{
	return false;
}

extern bool RLM3_I2C1_TransmitReceive(I2cAddress addr, const uint8_t* tx_data, size_t tx_size, uint8_t* rx_data, size_t rx_size)
{
	return false;
}
