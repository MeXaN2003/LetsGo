/*
 * DFPlayer.h
 *
 *  Created on: Apr 23, 2022
 *      Author: MeXaN
 */

#ifndef DFPLAYER_H_
#define DFPLAYER_H_
#include "main.h"
#define Source     0x08
#define Start_Byte 0x7E
#define End_Byte   0xEF
#define Version    0xFF
#define Cmd_Len    0x06
#define Feedback   0x00
#define delayDF	   250

enum PlayerStatus : uint8_t{
	PAUSE,
	PLAYING,
	VOLUME,
	RESETING
};

class DFPlayer {
public:
	DFPlayer(UART_HandleTypeDef *huart);
	void Next();
	void Pause();
	void Previous();
	void Playback();
	void Play();
	void Reseting();
	void Volume(uint8_t volume);
	bool Ready();
	void SetStatus();
	PlayerStatus GetNowStatus();
	PlayerStatus status = PAUSE;
	uint8_t volumeSet;
private:
	uint8_t volumeNow = 0;
	uint32_t timer = 0;
	PlayerStatus nowStatus = RESETING;
	UART_HandleTypeDef *huartDF;
	void Send_cmd(uint8_t cmd, uint8_t Parameter1, uint8_t Parameter2);
};
#endif /* DFPLAYER_H_ */
