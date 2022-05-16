/*
 * MexButton.h
 *
 *  Created on: 25 апр. 2022 г.
 *      Author: MeXaN
 */

#ifndef MEXBUTTON_H_
#define MEXBUTTON_H_
#include "main.h"
#include "stdint.h"

#define _buttonRead() HAL_GPIO_ReadPin(Button_Port, Button_Pin)
#define HIGH_PULL 0
#define LOW_PULL 1
#define NORM_OPEN 0
#define NORM_CLOSE 1
#define MANUAL 0
#define AUTO 1

#pragma pack(push,1)
typedef struct {
	bool btn_deb :1;
	bool hold_flag :1;
	bool counter_flag :1;
	bool isHolded_f :1;
	bool isRelease_f :1;
	bool isPress_f :1;
	bool step_flag :1;
	bool oneClick_f :1;
	bool isOne_f :1;
	bool inv_state :1;
	bool mode :1;
	bool type :1;
	bool tickMode :1;
	bool noPin :1;
	bool counter_reset :1;
} MexButtonFlags;
#pragma pack(pop)
class MexButton {
public:
	MexButton(GPIO_TypeDef *Button_Port_Constr, uint16_t Button_Pin_Constr,
			bool type = HIGH_PULL, bool dir = NORM_OPEN);
	void setDebouns(uint16_t debounse);
	void setTimeout(uint16_t new_timeout);
	void setClickTimeout(uint16_t new_timeout);
	void setStepTimeout(uint16_t step_timeout);
	void setType(bool type);
	void setDirection(bool dir);
	void setTickMode(bool tickMode);
	void tick();
	void tick(bool state);
	bool isPress();
	bool isRelease();
	bool isClick();
	bool isHolded();
	bool isHold();
	bool state();
	bool isSingle();
	bool isDouble();
	bool isTriple();
	bool hasClicks();
	uint8_t getClicks();
	uint8_t getHoldClicks();
	bool isStep(uint8_t clicks = 0);
	void resetStates();
private:
	MexButtonFlags flags;
	GPIO_TypeDef *Button_Port;
	uint16_t Button_Pin;
	uint16_t _debounce = 60;
	uint16_t _timeout = 500;
	uint16_t _click_timeout = 500;
	uint16_t _step_timeout = 400;
	uint8_t btn_counter = 0, last_counter = 0, last_hold_counter = 0;
	uint32_t btn_timer = 0;
	bool btn_state = false;
	bool btn_flag = false;
};

#endif /* MEXBUTTON_H_ */
