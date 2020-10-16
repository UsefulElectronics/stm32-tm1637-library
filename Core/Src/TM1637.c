/*______________In the name of Allah, Most Gracious, Most Merciful______________
	
	This library is written to allow the STM32 micro-controller to handle I2C
	communication protocol manually (through GPIO pins)
	
	Created by 				: Ward Almasarani 
	Start Date 				: 2020/09/29
	file name 				: tm1637.c
	Version					: 1.0
______________________________________________________________________________*/

#ifndef tm1637
#include "tm1637.h"
#include "stm32f1xx_hal_gpio.h"
void tm1637_CLKhigh()
{																	  //CLK period fix
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
}
void tm1637_CLKlow()
{
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
}
void tm1637_SDOhigh()
{
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);
}
void tm1637_SDOlow()
{
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);
}
void tm1637_SartPacket()
{
	tm1637_CLKhigh();

	tm1637_SDOhigh();
	tm1637_SDOlow();

	tm1637_CLKlow();
}
void tm1637_EndPacket()
{
	tm1637_CLKlow();
	tm1637_SDOlow();

	tm1637_CLKhigh();
	tm1637_SDOhigh();
}
void tm1637_DataOut(uint8_t *tm1637_TxBuffer)									
{
//	for(int8_t j = 7; j >= 0; j--)
	for(int8_t j = 0; j < PACKET_SIZE; j++)
	{
		tm1637_CLKlow();
		if(tm1637_TxBuffer[j] == GPIO_PIN_SET)
		{
			tm1637_SDOhigh();
//			tm1637_CLK();
		}
		else
		{
			tm1637_SDOlow();
//			tm1637_CLK();
		}
		tm1637_CLKhigh();
	} 
}
void tm1637_TxCommand(uint8_t *Command)
{																		//Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};

//	for(int8_t i = 7; i >= 0; i--)
	for(uint8_t i = 0; i < PACKET_SIZE; i++)
	{

		ByteData[i] = (Command[0] & (0x01 << i)) && 1;

												//Transmit byte by byte
	}

	tm1637_SartPacket();
	tm1637_DataOut(ByteData);
	tm1637_CLKlow();
	tm1637_CLKhigh();
	tm1637_ACKcheck();

//	tm1637_CLK();
//	if(Command[0] != C0H)
//	{
////		tm1637_CLK();
//		tm1637_EndPacket();
//
//	}



}
void tm1637_TxData(uint8_t Command, uint8_t *Data, uint8_t PacketSize)
{																		//Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};
//	if(Command != tm1637_DATA)
//	ByteData[0] = Command;
//	for(uint8_t j = 1; j < 8; j++)
//	{
//		ByteData[j] = (Data[j] & (0x01 << j)) && 1;				//Convert Byte to bit
//	}
//	tm1637_DataOut(ByteData);												//Send Slave address
//	tm1637_SartPacket();
//	for(int8_t i = PacketSize-1; i >= 0; i--)							//MSB firs
	for(uint8_t i = 0; i < PacketSize; i++)
	{
		for(uint8_t j = 0; j < 8; j++)
		{
			ByteData[j] = (Data[i] & (0x01 << j)) && 1;
		}
		tm1637_DataOut(ByteData);
		tm1637_CLKlow();
		tm1637_CLKhigh();
		tm1637_ACKcheck();//Transmit byte by byte
//		HAL_Delay(1);
	}
//	tm1637_CLK();
//	tm1637_ACKcheck();


}
void tm1637_Initialize(uint8_t Direction)						//Since SDI line is doing both transmission and reception
{																		//the corresponding GPIO pin must be reinitialized on the run
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = SCLK_Pin;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	switch (Direction)
	{
		case DISPLAY2STM:
			GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
			break;
		case STM2DISPLAY:
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
			break;
		default:
			break;
	}

}
void tm1637_ACKcheck()
{
	tm1637_Initialize(DISPLAY2STM);									//
	tm1637_CLKlow();
	while(HAL_GPIO_ReadPin(SCLK_GPIO_Port, SCLK_Pin))
	tm1637_Initialize(STM2DISPLAY);


}
#endif
