/*______________In the name of Allah, Most Gracious, Most Merciful______________
	
	This library is written to allow the STM32 micro-controller to handle TM1637 LED
	driver using a manual communication protocol (through GPIO pins)
	
	Created by 				: Ward Almasarani 
	Start Date 				: 2020/09/29
	file name 				: tm1637.c
	Version					: 1.0
______________________________________________________________________________*/

#ifndef tm1637
#include "tm1637.h"
#include "stm32f1xx_hal_gpio.h"

extern uint32_t Timer1;
extern uint8_t CurrentDisplay[4];
extern uint8_t tm1637_Segments[8];
void tm1637_CLKhigh()
{																	  	//SCL high period
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);		  	//Setting SCL frequency
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_SET);
}
void tm1637_CLKlow()
{
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);		//SCL low period
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SCLK_GPIO_Port, SCLK_Pin, GPIO_PIN_RESET);
}
void tm1637_SDOhigh()
{
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);			//SDO high period
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_SET);
}
void tm1637_SDOlow()
{
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);			//SDO low period
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SDO_GPIO_Port, SDO_Pin, GPIO_PIN_RESET);
}
void tm1637_StartPacket()												//Lower SDO line while CLK line is high
{
	tm1637_CLKhigh();

	tm1637_SDOhigh();
	tm1637_SDOlow();

	tm1637_CLKlow();
}
void tm1637_EndPacket()													//SDO line is pulled high while SCL line is high
{
	tm1637_CLKlow();
	tm1637_SDOlow();

	tm1637_CLKhigh();
	tm1637_SDOhigh();
}
void tm1637_DataOut(uint8_t *tm1637_TxBuffer)							//Low level data transfer function
{

	for(int8_t j = 0; j < PACKET_SIZE; j++)								//Send least significant bit first
	{
		tm1637_CLKlow();
		if(tm1637_TxBuffer[j] == GPIO_PIN_SET)							//Check logic level
		{
			tm1637_SDOhigh();

		}
		else
		{
			tm1637_SDOlow();

		}
		tm1637_CLKhigh();
	} 
}
void tm1637_TxCommand(uint8_t *Command)
{																		//Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};

	for(uint8_t i = 0; i < PACKET_SIZE; i++)
	{

		ByteData[i] = (Command[0] & (0x01 << i)) && 1;

																		//Convert from byte to bit per array element
	}

	tm1637_StartPacket();												//Send start packet bit
	tm1637_DataOut(ByteData);											//Send one byte
	tm1637_CLKlow();													//Send one CLK for acknowledgment
	tm1637_CLKhigh();
	tm1637_ACKcheck();													//wait for acknowledgment.
	if((Command[0] & 0xC0) != (0xC0))										//Check if the received packet is not an address.
	{
		tm1637_EndPacket();
	}

}
void tm1637_TxData(uint8_t *Data, uint8_t PacketSize)
{																		//Handles high level (bit by bit) transmission operation
	uint8_t ByteData[8] = {0};

	for(uint8_t i = 0; i < PacketSize; i++)
	{
		for(uint8_t j = 0; j < 8; j++)
		{
			ByteData[j] = (Data[i] & (0x01 << j)) && 1;
		}
		tm1637_DataOut(ByteData);
		tm1637_CLKlow();
		tm1637_CLKhigh();
		tm1637_ACKcheck();												//Transmit byte by byte

	}
	tm1637_EndPacket();													//Send end packet at the end of data transmission.


}
void tm1637_Initialize(uint8_t Direction)								//Since SDI line is doing both transmission and reception
{																		//the corresponding GPIO pin must be reinitialized on the run
	GPIO_InitTypeDef GPIO_InitStruct = {0};								//To read ACK from TM1637 and to write data to it
	GPIO_InitStruct.Pin = SCLK_Pin;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	switch (Direction)													//Depending on the function input initialize the pin as input or output
	{
		case DISPLAY2STM:
			GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
			break;
		case STM2DISPLAY:
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
			break;

	}

}
void tm1637_ACKcheck()
{
	//Wait for acknowledgment bit
	tm1637_Initialize(DISPLAY2STM);										//initialize pin as input
	tm1637_CLKlow();													//lower CLK line
	while(HAL_GPIO_ReadPin(SCLK_GPIO_Port, SCLK_Pin))					//Wait until ACK bit is received
	tm1637_Initialize(STM2DISPLAY);										//initialize pin as output for data transfer
}
void tm1637_DisplayClear()
{
	uint8_t EmptyBuffer[4] = {0};
	uint8_t CommandCarrier[1] = {0};
	CommandCarrier[0] = DATA_SET;									//Send set data command
	tm1637_TxCommand(CommandCarrier);
	CommandCarrier[0] = C0H;										//Set address
	tm1637_TxCommand(CommandCarrier);
	tm1637_TxData(EmptyBuffer, 4);
	CommandCarrier[0] = DISPLAY_OFF;
	tm1637_TxCommand(CommandCarrier);
}
uint8_t tm1637_DisplayHandle(uint8_t Brightness, uint8_t *DisplayBuffer)
{
	//This function handles the low level protocol used to set data address of TM1637 and turn the display on
	//#param Brightness is used to set the brightness level of the display. This function accepts Brightness value between 0 and 7
	//#param *DisplayBuffer is the buffer used to map data from the RAM to the display each element corresponds to one segment in the display
	uint8_t CommandCarrier[1] = {0};
	tm1637_StatusTypedef ParameterFalidation = TM1637_ERROR;
	if(Brightness <= 7)												//there are 7 levels of brightness
	{
	  CommandCarrier[0] = DATA_SET;									//Send set data command
	  tm1637_TxCommand(CommandCarrier);
	  CommandCarrier[0] = C0H;										//Set address
	  tm1637_TxCommand(CommandCarrier);

	  tm1637_TxData(DisplayBuffer, 4);								//Map the data stored in RAM to the display
	  tm1637_SetBrighness(Brightness);								//Turn on display and set brightness
		ParameterFalidation = TM1637_OK;
		return ParameterFalidation;
	}
	return ParameterFalidation;
}
tm1637_StatusTypedef tm1637_SetBrighness(uint8_t BrighnessLevel)
{
	uint8_t BrighnessBuffer[8] = {0};
	tm1637_StatusTypedef ParameterFalidation = TM1637_ERROR;
	if(BrighnessLevel <= 7)												//there are 7 levels of brightness
	{																	//Any value above that will be ignored.
		BrighnessLevel = BrighnessLevel | DISPLAY_ON;					//Set Brightness level with display on command

		for(uint8_t i = 0; i < 8; i++)
		{
			BrighnessBuffer[i] = (BrighnessLevel & (0x01 << i)) && 1;
		}
		tm1637_StartPacket();
		tm1637_DataOut(BrighnessBuffer);
		tm1637_CLKlow();													//Send one CLK for acknowledgment
		tm1637_CLKhigh();
		tm1637_ACKcheck();													//wait for acknowledgment.
		tm1637_EndPacket();
		ParameterFalidation = TM1637_OK;
		return ParameterFalidation;
	}
	return ParameterFalidation;
}
void tm1637_Effect(uint8_t Rotationfrequency)
{
	  static uint8_t SegmentTracker = 0;
	  static uint8_t SegmentPointer = 0;
	  if(Timer1 >= 150)
	  {
		  Timer1 = 0;
		  if(SegmentTracker < 4)
		  {
			  CurrentDisplay[0] = 0;
			  CurrentDisplay[1] = 0;
			  CurrentDisplay[2] = 0;
			  CurrentDisplay[3] = tm1637_Segments[SegmentPointer];
			  if(SegmentPointer != 3)
			  {
				  SegmentPointer ++;									//Prevent extra incrementing
			  }
		  }
		  else if((SegmentTracker > 5) && (SegmentTracker < 10))
		  {
			  if(SegmentPointer == 6)
			  {
				  SegmentPointer = 0;
			  }
			  CurrentDisplay[0] = tm1637_Segments[SegmentPointer];
			  CurrentDisplay[1] = 0;
			  CurrentDisplay[2] = 0;
			  CurrentDisplay[3] = 0;

			  SegmentPointer ++;
		  }
		  else if(SegmentTracker == 4)
		  {
			  CurrentDisplay[0] = 0;
			  CurrentDisplay[1] = 0;
			  CurrentDisplay[2] = tm1637_Segments[SegmentPointer];
			  CurrentDisplay[3] = 0;
		  }
		  else if(SegmentTracker == 5)
		  {
			  CurrentDisplay[0] = 0;
			  CurrentDisplay[1] = tm1637_Segments[SegmentPointer];
			  CurrentDisplay[2] = 0;
			  CurrentDisplay[3] = 0;
		  }
		  else if(SegmentTracker == 10)
		  {
			  SegmentPointer = 0;
			  CurrentDisplay[0] = 0;
			  CurrentDisplay[1] = tm1637_Segments[SegmentPointer];
			  CurrentDisplay[2] = 0;
			  CurrentDisplay[3] = 0;
		  }
		  else if(SegmentTracker == 11)
		  {
			  CurrentDisplay[0] = 0;
			  CurrentDisplay[1] = 0;
			  CurrentDisplay[2] = tm1637_Segments[SegmentPointer];
			  CurrentDisplay[3] = 0;
		  }
		  tm1637_DisplayHandle(7, CurrentDisplay);
		  SegmentTracker++;
		  if(SegmentTracker > 11)										//Once 12 steps are completed reset all trackers.
		  {
			  SegmentPointer = 0;
			  SegmentTracker = 0;
		  }

	  }
}

uint8_t char2segments(char c) {
        switch (c) {
			case '0' : return 0x3f;
			case '1' : return 0x06;
			case '2' : return 0x5b;
			case '3' : return 0x4f;
			case '4' : return 0x66;
			case '5' : return 0x6d;
			case '6' : return 0x7d;
			case '7' : return 0x07;
			case '8' : return 0x7f;
			case '9' : return 0x6f;
            case '_' : return 0x08;
            case '^' : return 0x01; // ¯
            case '-' : return 0x40;
            case '*' : return 0x63; // °
            case ' ' : return 0x00; // space
            case 'A' : return 0x77; // upper case A
            case 'a' : return 0x5f; // lower case a
            case 'B' :              // lower case b
            case 'b' : return 0x7c; // lower case b
            case 'C' : return 0x39; // upper case C
            case 'c' : return 0x58; // lower case c
            case 'D' :              // lower case d
            case 'd' : return 0x5e; // lower case d
            case 'E' :              // upper case E
            case 'e' : return 0x79; // upper case E
            case 'F' :              // upper case F
            case 'f' : return 0x71; // upper case F
            case 'G' :              // upper case G
            case 'g' : return 0x35; // upper case G
            case 'H' : return 0x76; // upper case H
            case 'h' : return 0x74; // lower case h
            case 'I' : return 0x06; // 1
            case 'i' : return 0x04; // lower case i
            case 'J' : return 0x1e; // upper case J
            case 'j' : return 0x16; // lower case j
            case 'K' :              // upper case K
            case 'k' : return 0x75; // upper case K
            case 'L' :              // upper case L
            case 'l' : return 0x38; // upper case L
            case 'M' :              // twice tall n
            case 'm' : return 0x37; // twice tall ∩
            case 'N' :              // lower case n
            case 'n' : return 0x54; // lower case n
            case 'O' :              // lower case o
            case 'o' : return 0x5c; // lower case o
            case 'P' :              // upper case P
            case 'p' : return 0x73; // upper case P
            case 'Q' : return 0x7b; // upper case Q
            case 'q' : return 0x67; // lower case q
            case 'R' :              // lower case r
            case 'r' : return 0x50; // lower case r
            case 'S' :              // 5
            case 's' : return 0x6d; // 5
            case 'T' :              // lower case t
            case 't' : return 0x78; // lower case t
            case 'U' :              // lower case u
            case 'u' : return 0x1c; // lower case u
            case 'V' :              // twice tall u
            case 'v' : return 0x3e; // twice tall u
            case 'W' : return 0x7e; // upside down A
            case 'w' : return 0x2a; // separated w
            case 'X' :              // upper case H
            case 'x' : return 0x76; // upper case H
            case 'Y' :              // lower case y
            case 'y' : return 0x6e; // lower case y
            case 'Z' :              // separated Z
            case 'z' : return 0x1b; // separated Z
        }
        return 0;
    }
#endif
