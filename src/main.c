#include "stm32f10x.h"
#include "stm32f1xx_nucleo.h"
#include "LIB_Config.h"

volatile uint16_t DMA_state_vector[3] = {0, 0, 0};
volatile uint8_t command_count = 0;

void send_char(char c)
{
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, c);
}

int __io_putchar(int c)
{
	if (c=='\n')
		send_char('\r');
	send_char(c);
	return c;
}

void DMA1_Channel6_IRQHandler()
{
	if (DMA_GetFlagStatus(DMA1_FLAG_TC6) == SET)
	{
		DMA_ClearFlag(DMA1_FLAG_TC6);
		DMA_Cmd(DMA1_Channel6, DISABLE);
		DMA_ClearFlag(DMA1_FLAG_TC6);
	}
}


void I2C1_EV_IRQHandler()
{
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == SET)
	{
		I2C_ClearFlag(I2C1, I2C_FLAG_SB);
		I2C_Send7bitAddress(I2C1,0x78,I2C_Direction_Transmitter);
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == SET)
	{
		I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);
		I2C1 -> SR2;
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == SET)
	{
		I2C_ClearFlag(I2C1, I2C_FLAG_BTF);
		I2C_GenerateSTOP(I2C1, ENABLE);

		if (DMA_state_vector[0] == 1)
			ssd1306_refresh_gram();
		else if (DMA_state_vector[0] == 2)
			ssd1306_clear_screen(fill);
		else if (screen_init == 0)
		{
			screen_init = 1;
			command_count++;
			program();
		}
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == SET)
			I2C_ClearFlag(I2C1, I2C_FLAG_RXNE);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADD10) == SET)
			I2C_ClearFlag(I2C1, I2C_FLAG_ADD10);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF) == SET)
			I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == SET)
			I2C_ClearFlag(I2C1, I2C_FLAG_TXE);
}

int main(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 | RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_2; // RES
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &gpio);

	GPIO_SetBits(GPIOC, GPIO_Pin_2);

	gpio.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7; // SCL, SDA
	gpio.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_6; // SCL, SDA
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &gpio);

	GPIO_ResetBits(GPIOC, GPIO_Pin_6);

	USART_InitTypeDef uart;
	USART_StructInit(&uart);

	uart.USART_BaudRate = 115200;
	USART_Init(USART2, &uart);
	USART_Cmd(USART2, ENABLE);

	DMA_StructInit(&dma);
	DMA_DeInit(DMA1_Channel6);

	dma.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DR;
	dma.DMA_DIR = DMA_DIR_PeripheralDST;
	dma.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
	dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma.DMA_M2M = DMA_M2M_Disable;
	dma.DMA_Mode = DMA_Mode_Normal;
	dma.DMA_Priority = DMA_Priority_VeryHigh;
	dma.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel6, &dma);

	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

	I2C_InitTypeDef i2c;
	I2C_StructInit(&i2c);

	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_OwnAddress1 = 0x03;
	i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c.I2C_Ack = I2C_Ack_Enable;
	i2c.I2C_ClockSpeed = 400000;
	i2c.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2C_Init(I2C1,&i2c);

	I2C_Cmd(I2C1, ENABLE);

	I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);

	NVIC_InitTypeDef nvic;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	nvic.NVIC_IRQChannel = I2C1_EV_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	nvic.NVIC_IRQChannel = DMA1_Channel6_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	program();

	while(1)
	{

	}
}
