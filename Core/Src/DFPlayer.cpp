/*
 * DFPlayer.cpp
 *
 *  Created on: Apr 23, 2022
 *      Author: MeXaN
 */

#include "DFPlayer.h"

DFPlayer::DFPlayer(UART_HandleTypeDef *huart) {
	huartDF = huart;
	Send_cmd(0x3F, 0x00, Source);
	HAL_Delay(200);
	Send_cmd(0x06, 0x00, 1);
	HAL_Delay(500);
	timer = HAL_GetTick();
}
void DFPlayer::Send_cmd(uint8_t cmd, uint8_t Parameter1, uint8_t Parameter2) {
	uint16_t Checksum = Version + Cmd_Len + cmd + Feedback + Parameter1
			+ Parameter2;
	Checksum = 0 - Checksum;

	uint8_t CmdSequence[10] = { Start_Byte, Version, Cmd_Len, cmd, Feedback,
			Parameter1, Parameter2, (Checksum >> 8) & 0x00ff,
			(Checksum & 0x00ff), End_Byte };

	HAL_UART_Transmit(huartDF, CmdSequence, 10, HAL_MAX_DELAY);
	timer = HAL_GetTick();
}

void DFPlayer::Next() {
	Send_cmd(0x01, 0x00, 0x00);
}
void DFPlayer::Pause() {
	Send_cmd(0x0E, 0, 0);
}

void DFPlayer::Reseting(){
	Send_cmd(0x0c, 0, 0);
}

PlayerStatus DFPlayer::GetNowStatus(){
	return nowStatus;
}

void DFPlayer::Previous() {
	Send_cmd(0x02, 0, 0);
}

void DFPlayer::Playback() {
	Send_cmd(0x18, 0, 0);
}

void DFPlayer::Play() {
	Send_cmd(0x0D, 0, 0);
}

void DFPlayer::Volume(uint8_t volume) {
	if(volume == volumeNow) return;
	if (volume > 30)
		volume = 30;
	Send_cmd(0x06, 0x00, volume);
	volumeNow = volume;
}

bool DFPlayer::Ready() {
	if (HAL_GetTick() - timer >= delayDF) {
		return true;
	} else
		return false;
}

void DFPlayer::SetStatus() {
	if (nowStatus == status) {
		return;
	} else {
		switch (status) {
		case PAUSE:
			Pause();
			break;
		case PLAYING:
			Play();
			break;
		case VOLUME:
			Volume(volumeSet);
			break;
		case RESETING:
			Pause();
			break;
		default:
			break;
		}
		nowStatus = status;
	}
}
