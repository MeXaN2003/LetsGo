/*
 * MexButton.cpp
 *
 *  Created on: 25 апр. 2022 г.
 *      Author: MeXaN
 */

#include "MexButton.h"

MexButton::MexButton(GPIO_TypeDef *Button_Port_Constr,
		uint16_t Button_Pin_Constr, bool type, bool dir) {
	if (NULL != Button_Port_Constr) {
		Button_Port = Button_Port_Constr;
		Button_Pin = Button_Pin_Constr;
		flags.noPin = 0;
	} else {
		flags.noPin = 1;
	}
	setType(type);
	flags.mode = 0;
	flags.tickMode = 0;
	flags.inv_state = dir;
}

void MexButton::setDebouns(uint16_t debounce) {
	_debounce = debounce;
}
void MexButton::setTimeout(uint16_t new_timeout) {
	_timeout = new_timeout;
}
void MexButton::setClickTimeout(uint16_t new_timeout) {
	_click_timeout = new_timeout;
}
void MexButton::setStepTimeout(uint16_t step_timeout) {
	_step_timeout = step_timeout;
}
void MexButton::setType(bool type) {
	flags.type = type;
	if (!flags.noPin) {
		GPIO_InitTypeDef GPIO_InitStruct = { 0 };
		GPIO_InitStruct.Pin = Button_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		if (type) {
			GPIO_InitStruct.Pull = GPIO_NOPULL;
		} else {
			GPIO_InitStruct.Pull = GPIO_PULLUP;
		}
		HAL_GPIO_Init(Button_Port, &GPIO_InitStruct);
	}
}

bool MexButton::isPress() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.isPress_f) {
		flags.isPress_f = 0;
		return 1;
	} else
		return 0;
}
bool MexButton::isRelease() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.isRelease_f) {
		flags.isRelease_f = 0;
		return 1;
	} else
		return 0;
}
bool MexButton::isClick() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.isOne_f) {
		flags.isOne_f = 0;
		return 1;
	} else
		return 0;
}
bool MexButton::isHolded() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.isHolded_f) {
		flags.isHolded_f = 0;
		return 1;
	} else
		return 0;
}
bool MexButton::isHold() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.step_flag) {
		return 1;
	} else
		return 0;
}
bool MexButton::state() {
	if (flags.tickMode)
		MexButton::tick();
	return btn_state;
}
bool MexButton::isSingle() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.counter_flag && last_counter == 1) {
		flags.counter_reset = 1;
		return 1;
	} else
		return 0;
}
bool MexButton::isDouble() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.counter_flag && last_counter == 2) {
		flags.counter_reset = 1;
		return 1;
	} else
		return 0;
}
bool MexButton::isTriple() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.counter_flag && last_counter == 3) {
		flags.counter_reset = 1;
		return 1;
	} else
		return 0;
}
bool MexButton::hasClicks() {
	if (flags.tickMode)
		MexButton::tick();
	if (flags.counter_flag) {
		flags.counter_reset = 1;
		return 1;
	} else
		return 0;
}
uint8_t MexButton::getClicks() {
	flags.counter_reset = 1;
	return last_counter;
}
uint8_t MexButton::getHoldClicks() {
	if (flags.tickMode)
		MexButton::tick();
	return last_hold_counter;
}
bool MexButton::isStep(uint8_t clicks) {
	if (flags.tickMode)
		MexButton::tick();
	if (btn_counter == clicks && flags.step_flag
			&& (HAL_GetTick() - btn_timer >= _step_timeout)) {
		btn_timer = HAL_GetTick();
		return 1;
	} else
		return 0;
}

void MexButton::resetStates() {
	flags.isPress_f = false;
	flags.isRelease_f = false;
	flags.isOne_f = false;
	flags.isHolded_f = false;
	flags.step_flag = false;
	flags.counter_flag = false;
	last_hold_counter = 0;
	last_counter = 0;
}

void MexButton::tick(bool state) {
	flags.mode = 1;
	btn_state = state ^ flags.inv_state;
	MexButton::tick();
	flags.mode = 0;
}

void MexButton::tick() {
	if (!flags.mode && !flags.noPin) {
		btn_state = !_buttonRead() ^ (flags.inv_state ^ flags.type);
	}
	uint32_t thisMls = HAL_GetTick();

	if (btn_state && !btn_flag) {
		if (!flags.btn_deb) {
			flags.btn_deb = 1;
			btn_timer = thisMls;
		} else {
			if (thisMls - btn_timer >= _debounce) {
				btn_flag = 1;
				flags.isPress_f = 1;
				flags.oneClick_f = 1;
			}
		}
	} else {
		flags.btn_deb = 0;
	}
	if (!btn_state && btn_flag) {
		btn_flag = 0;
		if (!flags.hold_flag)
			btn_counter++;
		flags.hold_flag = 0;
		flags.isRelease_f = 1;
		btn_timer = thisMls;
		if (flags.step_flag) {
			last_counter = 0;
			btn_counter = 0;
			flags.step_flag = 0;
		}
		if (flags.oneClick_f) {
			flags.oneClick_f = 0;
			flags.isOne_f = 1;
		}
	}
	if (btn_flag && btn_state && (thisMls - btn_timer >= _timeout)
			&& !flags.hold_flag) {
		flags.hold_flag = 1;
		last_hold_counter = btn_counter;
		flags.isHolded_f = 1;
		flags.step_flag = 1;
		flags.oneClick_f = 0;
		btn_timer = thisMls;
	}
	if ((thisMls - btn_timer >= _click_timeout) && (btn_counter != 0)
			&& !btn_state) {
		btn_counter = 0;
		flags.counter_flag = 1;
	}
	if (flags.counter_reset) {
		last_counter = 0;
		flags.counter_flag = 0;
		flags.counter_reset = 0;
	}
}

