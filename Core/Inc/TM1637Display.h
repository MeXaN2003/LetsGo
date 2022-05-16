/*
 * TM1637Display.h
 *
 *  Created on: Apr 15, 2022
 *      Author: MeXaN
 */

#ifndef TM1637DISPLAY_H_
#define TM1637DISPLAY_H_

#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44
#define STARTADDRES  0xc0
#define POINT_ON   1
#define POINT_OFF  0
#define  BRIGHT_DARKEST 0
#define  BRIGHT_TYPICAL 2
#define  BRIGHTEST      7
#include "stdint.h"
#include "stm32f1xx_hal.h"

class TM1637_Display{
public:
	TM1637_Display(GPIO_TypeDef *GPIOx_DIO, uint16_t GPIO_DIO_Pin,
			GPIO_TypeDef *GPIOx_CLK, uint16_t GPIO_CLK_Pin,
			TIM_HandleTypeDef *htim);
	void display(uint8_t DispData[]);
	void displayByte(uint8_t DispData[]);
	void display(uint8_t BitAddr, uint8_t DispData);
	void displayByte(uint8_t BitAddr, uint8_t DispData);
	void displayByte(uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3);
	void display(uint8_t bit0, uint8_t bit1, uint8_t bit2, uint8_t bit3);
	void clear(void);
	void brightness(uint8_t brightness, uint8_t = 0x40, uint8_t = 0xc0);
	void point(bool PointFlag);
	void displayClock(uint8_t hrs, uint8_t mins);
	void displayInt(int16_t value);
private:
	TIM_TypeDef *timer;
	GPIO_TypeDef *_GPIOx_DIO, *_GPIOx_CLK;
	uint16_t _GPIO_DIO_Pin, _GPIO_CLK_Pin;
	uint8_t lastData[4] = { 0 }, PointData = 0, Cmd_SetData, Cmd_SetAddr,
			Cmd_DispCtrl;
	bool writeByte(uint8_t wr_data);
	void sendByte(uint8_t BitAddr, int8_t sendData);
	void sendArray(uint8_t sendData[]);
	void update(void);
	void start(void);
	void stop(void);
};

#endif /* INC_TM1637DISPLAY_H_ */
