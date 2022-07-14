#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void RLM3_UART2_Init(uint32_t baud_rate);
extern void RLM3_UART2_Deinit();
extern bool RLM3_UART2_IsInit();

extern void RLM3_UART2_EnsureTransmit();

extern void RLM3_UART2_ReceiveCallback(uint8_t data);
extern bool RLM3_UART2_TransmitCallback(uint8_t* data_to_send);
extern void RLM3_UART2_ErrorCallback(uint32_t status_flags);


extern void RLM3_UART4_Init(uint32_t baud_rate);
extern void RLM3_UART4_Deinit();
extern bool RLM3_UART4_IsInit();

extern void RLM3_UART4_EnsureTransmit();

extern void RLM3_UART4_ReceiveCallback(uint8_t data);
extern bool RLM3_UART4_TransmitCallback(uint8_t* data_to_send);
extern void RLM3_UART4_ErrorCallback(uint32_t status_flags);


#ifdef __cplusplus
}
#endif
