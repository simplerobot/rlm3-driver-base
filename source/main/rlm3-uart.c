#include "rlm3-uart.h"
#include "stm32f427xx.h"
#include "main.h"
#include "Assert.h"
#include "rlm3-helper.h"


/*
 * This UART interface was written to provide a simple interface to the UARTs used on the RLM3 PCB.  This interface
 * makes no assumptions about how the data is used.  When it needs data, it asks the application for it.  When it
 * receives data, it sends it to the application.
 */


static void UART_Init(USART_TypeDef* uart, uint32_t baud_rate)
{
	ASSERT(baud_rate <= 10500000);

	SET_REGISTER_FLAGS(uart->CR1,
			FLAG(USART_CR1_UE, 0)); // Disable UART
	SET_REGISTER_FLAGS(uart->CR2,
			FLAG(USART_CR2_STOP,  0), // 1 Stop Bit
			FLAG(USART_CR2_LINEN, 0),  // Disable LIN mode
			FLAG(USART_CR2_CLKEN, 0)); // Disable flow control clock
	SET_REGISTER_FLAGS(uart->CR1,
			FLAG(USART_CR1_M,     0),  // 8 Data Bits
			FLAG(USART_CR1_PCE,   0),  // Parity control disabled
			FLAG(USART_CR1_TE,    0),  // Disable Transmit
			FLAG(USART_CR1_RE,    0),  // Disable Receive
			FLAG(USART_CR1_OVER8, 0)); // Oversample 16
	SET_REGISTER_FLAGS(uart->CR3,
			FLAG(USART_CR3_RTSE,  0),  // Disable RTS Flow Control
			FLAG(USART_CR3_CTSE,  0),  // Disable CTS Flow Control
			FLAG(USART_CR3_SCEN,  0),  // Disable smartcard mode
			FLAG(USART_CR3_HDSEL, 0),  // Disable half duplex mode
			FLAG(USART_CR3_IREN,  0)); // Disable IrDA mode

	uint32_t pclk_frequency = HAL_RCC_GetPCLK1Freq();
	uint32_t baud_div = (pclk_frequency + baud_rate / 2) / baud_rate;
	ASSERT(baud_div <= (USART_BRR_DIV_Mantissa | USART_BRR_DIV_Fraction));
	uint32_t reg = uart->BRR;
	MODIFY_REG(reg, USART_BRR_DIV_Mantissa | USART_BRR_DIV_Fraction, baud_div); // Set baud rate
	uart->BRR = reg;

	SET_REGISTER_FLAGS(uart->CR1,
			FLAG(USART_CR1_UE,     1),  // Enable UART
			FLAG(USART_CR1_TE,     1),  // Enable Transmit
			FLAG(USART_CR1_RE,     1),  // Enable Receive
			FLAG(USART_CR1_TXEIE,  1),  // Enable TXE (Transmit data register empty) interrupt
			FLAG(USART_CR1_RXNEIE, 1)); // Enable RXNE (Read register not empty) interrupt
	SET_REGISTER_FLAGS(uart->CR3,
			FLAG(USART_CR3_EIE, 1)); // Enable ERR (Error) interrupt
}

static void UART_Deinit(USART_TypeDef* uart)
{
	SET_REGISTER_FLAGS(uart->CR1,
			FLAG(USART_CR1_UE, 0)); // Disable UART
}

static void UART_EnsureTransmit(USART_TypeDef* uart)
{
	SET_REGISTER_FLAGS(uart->CR1,
			FLAG(USART_CR1_TXEIE,  1)); // Enable TXE (Transmit data register empty) interrupt
}

extern void RLM3_UART2_Init(uint32_t baud_rate)
{
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = GPS_TX_Pin | GPS_RX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);

	UART_Init(USART2, baud_rate);
}

extern void RLM3_UART2_Deinit()
{
	UART_Deinit(USART2);

	__HAL_RCC_USART2_CLK_DISABLE();

	HAL_GPIO_DeInit(GPIOA, GPS_TX_Pin | GPS_RX_Pin);

	HAL_NVIC_DisableIRQ(USART2_IRQn);
}

extern bool RLM3_UART2_IsInit()
{
	return __HAL_RCC_USART2_IS_CLK_ENABLED();
}

extern void RLM3_UART2_EnsureTransmit()
{
	UART_EnsureTransmit(USART2);
}


extern void RLM3_UART4_Init(uint32_t baud_rate)
{
	__HAL_RCC_UART4_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = WIFI_TX_Pin | WIFI_RX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(UART4_IRQn);

	UART_Init(UART4, baud_rate);
}

extern void RLM3_UART4_Deinit()
{
	UART_Deinit(UART4);

	__HAL_RCC_UART4_CLK_DISABLE();

	HAL_GPIO_DeInit(GPIOA, WIFI_TX_Pin | WIFI_RX_Pin);

	HAL_NVIC_DisableIRQ(UART4_IRQn);
}

extern bool RLM3_UART4_IsInit()
{
	return __HAL_RCC_UART4_IS_CLK_ENABLED();
}

extern void RLM3_UART4_EnsureTransmit()
{
	UART_EnsureTransmit(UART4);
}

void USART2_IRQHandler(void)
{
	USART_TypeDef* uart = USART2;
	uint32_t SR = uart->SR;
	uint32_t CR1 = uart->CR1;

	if ((SR & USART_SR_RXNE) != 0 && (CR1 & USART_CR1_RXNEIE) != 0)
	{
		RLM3_UART2_ReceiveCallback(uart->DR & 0xFF);
	}
	if ((SR & USART_SR_TXE) != 0 && (CR1 & USART_CR1_TXEIE) != 0)
	{
		uint8_t data = 0;
		if (RLM3_UART2_TransmitCallback(&data))
		{
			uart->DR = data;
		}
		else
		{
			SET_REGISTER_FLAGS(uart->CR1,
					FLAG(USART_CR1_TXEIE,  0)); // Disable TXE (Transmit data register empty) interrupt
		}
	}
	if ((SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0)
	{
		RLM3_UART2_ErrorCallback(SR);
	}
}

void UART4_IRQHandler(void)
{
	USART_TypeDef* uart = UART4;
	uint32_t SR = uart->SR;
	uint32_t CR1 = uart->CR1;

	if ((SR & USART_SR_RXNE) != 0 && (CR1 & USART_CR1_RXNEIE) != 0)
	{
		RLM3_UART4_ReceiveCallback(uart->DR & 0xFF);
	}
	if ((SR & USART_SR_TXE) != 0 && (CR1 & USART_CR1_TXEIE) != 0)
	{
		uint8_t data = 0;
		if (RLM3_UART4_TransmitCallback(&data))
		{
			uart->DR = data;
		}
		else
		{
			SET_REGISTER_FLAGS(uart->CR1,
					FLAG(USART_CR1_TXEIE,  0)); // Disable TXE (Transmit data register empty) interrupt
		}
	}
	if ((SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0)
	{
		RLM3_UART4_ErrorCallback(SR);
	}
}


extern __weak void RLM3_UART2_ReceiveCallback(uint8_t data)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
}

extern __weak bool RLM3_UART2_TransmitCallback(uint8_t* data_to_send)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
	return false;
}

extern __weak void RLM3_UART2_ErrorCallback(uint32_t status_flags)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
}

extern __weak void RLM3_UART4_ReceiveCallback(uint8_t data)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
}

extern __weak bool RLM3_UART4_TransmitCallback(uint8_t* data_to_send)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
	return false;
}

extern __weak void RLM3_UART4_ErrorCallback(uint32_t status_flags)
{
	// DO NOT MODIFIY THIS FUNCTION.  Override it by declaring a non-weak version in your project files.
}
