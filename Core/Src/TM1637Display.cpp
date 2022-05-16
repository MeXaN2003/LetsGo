/*
 * TM1637Display.cpp
 *
 *  Created on: Apr 15, 2022
 *      Author: MeXaN
 */
#include "TM1637Display.h"

const uint8_t digitHEX[12] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
		0x7f, 0x6f, 0x00, 0x40 };
TM1637_Display::TM1637_Display(GPIO_TypeDef *GPIOx_DIO, uint16_t GPIO_DIO_Pin,
		GPIO_TypeDef *GPIOx_CLK, uint16_t GPIO_CLK_Pin,
		TIM_HandleTypeDef *htim) {
	_GPIOx_DIO = GPIOx_DIO;
	_GPIO_DIO_Pin = GPIO_DIO_Pin;
	_GPIOx_CLK = GPIOx_CLK;
	_GPIO_CLK_Pin = GPIO_CLK_Pin;
	timer = htim->Instance;
	HAL_TIM_Base_Start(htim);

}
void TM1637_Display::display(uint8_t DispData[]) {
	uint8_t SegData[4];
	for (uint8_t i = 0; i < 4; i++) {
		lastData[i] = digitHEX[DispData[i]];
		SegData[i] = digitHEX[DispData[i]] + PointData;
	}
	sendArray(SegData);
}

void TM1637_Display::displayByte(uint8_t DispData[]) {
	uint8_t SegData[4];
	for (uint8_t i = 0; i < 4; i++) {
		lastData[i] = DispData[i];
		SegData[i] = DispData[i] + PointData;
	}
	sendArray(SegData);
}

void TM1637_Display::display(uint8_t BitAddr, uint8_t DispData) {
	uint8_t SegData;
	lastData[BitAddr] = digitHEX[DispData];
	SegData = digitHEX[DispData] + PointData;
	sendByte(BitAddr, SegData);
}

void TM1637_Display::displayByte(uint8_t BitAddr, uint8_t DispData) {
	uint8_t SegData;
	lastData[BitAddr] = DispData;
	SegData = DispData + PointData;
	sendByte(BitAddr, SegData);
}

void TM1637_Display::displayByte(uint8_t bit0, uint8_t bit1, uint8_t bit2,
		uint8_t bit3) {
	uint8_t dispArray[4] = { bit0, bit1, bit2, bit3 };
	displayByte(dispArray);
}

void TM1637_Display::display(uint8_t bit0, uint8_t bit1, uint8_t bit2,
		uint8_t bit3) {
	uint8_t dispArray[] = { bit0, bit1, bit2, bit3 };
	display(dispArray);
}

void TM1637_Display::clear(void) {
	display(0x00, 0x7f);
	display(0x01, 0x7f);
	display(0x02, 0x7f);
	display(0x03, 0x7f);
	lastData[0] = 0x00;
	lastData[1] = 0x00;
	lastData[2] = 0x00;
	lastData[3] = 0x00;
}

void TM1637_Display::brightness(uint8_t brightness, uint8_t SetData,
		uint8_t SetAddr) {
	Cmd_SetData = 0x40;
	Cmd_SetAddr = 0xc0;
	Cmd_DispCtrl = 0x88 + brightness; //Set the brightness and it takes effect the next time it displays.
	update();
}

void TM1637_Display::point(bool PointFlag) {
	if (PointFlag)
		PointData = 0x80;
	else
		PointData = 0;
	update();
}

void TM1637_Display::displayClock(uint8_t hrs, uint8_t mins) {
	if (hrs > 99 || mins > 99)
		return;
	uint8_t disp_time[4];
	if ((hrs / 10) == 0)
		disp_time[0] = 10;
	else
		disp_time[0] = (hrs / 10);
	disp_time[1] = hrs % 10;
	disp_time[2] = mins / 10;
	disp_time[3] = mins % 10;
	display(disp_time);
}

void TM1637_Display::displayInt(int16_t value) {
	if (value > 9999 || value < -999)
		return;
	bool negative = false;
	uint8_t digits[4];
	if (value < 0) {
		negative = true;
		value = -value;
	}
	digits[0] = (int) value / 1000;
	uint16_t b = (int) digits[0] * 1000;
	digits[1] = ((int) value - b) / 100;
	b += digits[1] * 100;
	digits[2] = (int) (value - b) / 10;
	b += digits[2] * 10;
	digits[3] = value - b;
	if (!negative) {
		for (uint8_t i = 0; i < 3; i++) {
			if (digits[i] == 0)
				digits[i] = 10;
			else
				break;
		}
	} else {
		for (uint8_t i = 0; i < 3; i++) {
			if (digits[i] == 0) {
				if (digits[i + 1] == 0) {
					digits[i] = 10;
				} else {
					digits[i] = 11;
					break;
				}
			}
		}
	}
	display(digits);

}

bool TM1637_Display::writeByte(uint8_t wr_data) {
	uint8_t i;
	for (i = 0; i < 8; i++) {
		HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_RESET);

		if (wr_data & 0x01)
			HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_RESET);

		wr_data >>= 1;
		HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_SET);

	}
	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_SET);

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = _GPIO_DIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(_GPIOx_DIO, &GPIO_InitStruct);
	timer->CNT = 0;
	while (timer->CNT < 50) {
	}
	bool ack = HAL_GPIO_ReadPin(_GPIOx_DIO, _GPIO_DIO_Pin);
	if (ack == 0) {
		GPIO_InitTypeDef GPIO_InitStruct = { 0 };
		GPIO_InitStruct.Pin = _GPIO_DIO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(_GPIOx_DIO, &GPIO_InitStruct);
		HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_RESET);
		timer->CNT = 0;
		while (timer->CNT < 50) {
		}
	}

	return ack;
}

void TM1637_Display::sendByte(uint8_t BitAddr, int8_t sendData) {
	start();
	writeByte(ADDR_FIXED);
	stop();
	start();
	writeByte(BitAddr | 0xc0);
	writeByte(sendData);
	stop();
	start();
	writeByte (Cmd_DispCtrl);
	stop();
}

void TM1637_Display::sendArray(uint8_t sendData[]) {
	start();          //start signal sent to GyverTM1637 from MCU
	writeByte(ADDR_AUTO);          //
	stop();           //
	start();          //
	writeByte (Cmd_SetAddr);          //
	for (uint8_t i = 0; i < 4; i++) {
		writeByte(sendData[i]);        //
	}
	stop();           //
	start();          //
	writeByte (Cmd_DispCtrl);          //
	stop();
}

void TM1637_Display::update(void) {
	displayByte (lastData);
}

void TM1637_Display::start(void) {
	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_RESET);

}

void TM1637_Display::stop(void) {
	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(_GPIOx_CLK, _GPIO_CLK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(_GPIOx_DIO, _GPIO_DIO_Pin, GPIO_PIN_SET);
}



