#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void RLM3_UART2_Init();
extern void RLM3_UART2_Deinit();
extern bool RLM3_UART2_IsInit();

extern bool RLM3_UART2_Transmit(const uint8_t* data, size_t size);
extern bool RLM3_UART2_Receive(const uint8_t** data_out, size_t* size_out);
extern bool RLM3_UART2_ReceiveWithTimeout(const uint8_t** data_out, size_t* size_out, size_t timeout_ms);

extern void RLM3_UART4_Init();
extern void RLM3_UART4_Deinit();
extern bool RLM3_UART4_IsInit();

extern bool RLM3_UART4_Transmit(const uint8_t* data, size_t size);
extern bool RLM3_UART4_Receive(const uint8_t** data_out, size_t* size_out);
extern bool RLM3_UART4_ReceiveWithTimeout(const uint8_t** data_out, size_t* size_out, size_t timeout_ms);


#ifdef __cplusplus
}
#endif
