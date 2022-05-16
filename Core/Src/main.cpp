/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.cpp
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "TM1637Display.h"
#include "DFPlayer.h"
#include "MexButton.h"
#include "FlashPROM.h"
#include "sk6812.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUMBER_ALARMS 9
#define USER_INACTIVITY_TIME 10000
#define LAUNCH_TIME 4000
#define BLINK_TIMER 500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
RTC_TimeTypeDef alarms[NUMBER_ALARMS];
uint32_t res_addr;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
RTC_TimeTypeDef remainingTime(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b);
enum Mode : uint8_t {
	NORMAL, SET_CLOCK, SET_ALARM, CHOOSING_ALARM
};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_RTC_Init();
	MX_TIM4_Init();
	MX_USART1_UART_Init();
	MX_TIM2_Init();
	MX_DMA_Init();
	MX_TIM1_Init();
	MX_CRC_Init();
	/* USER CODE BEGIN 2 */
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	myBuf_t wdata[BUFFSIZE];
	for (uint8_t i = 0; i < BUFFSIZE; i++) {
		wdata[i] = 0;
	}
	res_addr = flash_search_adress(STARTADDR, BUFFSIZE * DATAWIDTH);
	myBuf_t rdata[BUFFSIZE];
	read_last_data_in_flash(rdata);
	for (uint8_t i = 0; i < BUFFSIZE - (BUFFSIZE - NUMBER_ALARMS); i++) {
		alarms[i].Hours = rdata[i] >> 16;
		alarms[i].Minutes = rdata[i] >> 8;
		if (alarms[i].Seconds >= 60) {
			alarms[i].Seconds = 0;
		} else {
			alarms[i].Seconds = rdata[i];
		}
	}
	/*
	 * uint8_t mode = 0;
	 * 1 - setting up an alarm clock
	 * 2 - setting up the clock
	 * 3 - setting an alarm
	 */
	uint32_t setTimer = HAL_GetTick();
	uint32_t scrollTimer = HAL_GetTick();
	uint8_t sets, setsM, setsH, setsDelay, setingAlarm, ready, numberNowAlarm =
			(uint8_t) rdata[BUFFSIZE - 2];
	bool on, blink = 0, plaing = 0, excited = (uint8_t) (rdata[BUFFSIZE - 2]
			>> 16);
	Mode mode = NORMAL;
	//bool alarmed = (uint8_t) (rdata[BUFFSIZE - 2] >> 8);
	bool alarmed = false;
	bool changes = false;
	uint32_t timer = HAL_GetTick();
	RTC_TimeTypeDef nowTime = { 0 };
	RTC_TimeTypeDef nowAlarm = alarms[numberNowAlarm], timeToAlarm;
	/*
	 * setTimer - timer for blinking the display when setting up
	 * scrollTimer - timer for rewinding digits during installation
	 * sets - variable submenu time settings, etc.
	 * setsM, setsH - variables, for setting hours and minutes
	 * setingAlarm - variable for selecting the alarm number
	 * on - flag for blinking the display
	 * excited - flag for turning on the alarm
	 * alarmed - flag, triggered while the alarm is running
	 * timer - timer variable for exiting the settings when the user is idle
	 * nowTime - time structure for getting the time from the RTC and output to the display
	 * nowAlarm - time structure for the alarm clock currently set
	 * timeToAlarm - remaining time until the alarm clock
	 */
	TM1637_Display display(Display_DIO_GPIO_Port,
	Display_DIO_Pin,
	Display_CLK_GPIO_Port, Display_CLK_Pin, &htim4);
	DFPlayer player(&huart1);
	MexButton BT_Plus(Button_3_GPIO_Port, Button_3_Pin);
	MexButton BT_Set(Button_2_GPIO_Port, Button_2_Pin);
	MexButton BT_Minus(Button_1_GPIO_Port, Button_1_Pin);
	BT_Set.setTimeout(1200);
	BT_Plus.setDebouns(60);
	BT_Minus.setDebouns(60);
	BT_Set.setDebouns(60);
	display.clear();
	display.brightness(2);
	player.Volume(0);
	player.status = PAUSE;
	HAL_GPIO_WritePin(LED_Alarm_GPIO_Port, LED_Alarm_Pin, GPIO_PIN_RESET);
	led_set_all_RGB((RGB_t ) { 0, 30, 5 });
	led_render();
	HAL_Delay(400);
	led_set_all_RGB((RGB_t ) { 0, 0, 0 });
	led_render();
	while (true) {

		if (player.Ready()) {
			player.SetStatus();
			if (player.GetNowStatus() == PLAYING) {
				plaing = true;
			}
			if (player.GetNowStatus() == PAUSE) {
				plaing = false;
			}
		}

		if (HAL_GetTick() >= LAUNCH_TIME) {
			BT_Plus.tick();
			BT_Set.tick();
			BT_Minus.tick();
		} else {
			BT_Plus.resetStates();
			BT_Set.resetStates();
			BT_Minus.resetStates();
		}
		if (nowTime.Seconds % 15 >= 14) {
			timeToAlarm = remainingTime(&nowAlarm, &nowTime);
		}

		if (timeToAlarm.Hours == 0 && timeToAlarm.Minutes <= nowAlarm.Seconds
				&& excited) {
			ready = 16
					- ((uint16_t) (timeToAlarm.Minutes * 60 - nowTime.Seconds)
							* 4) / (uint16_t) (nowAlarm.Seconds * 15);
			if (!plaing) {
				player.status = PLAYING;
			} else {
				player.volumeSet = ready + 10;
				player.status = VOLUME;
			}
			for (int i = 0; i < NUM_PIXELS / 2; i++) {
				HSV_t a;
				a.h = i / 5;
				a.s = 15;
				a.v = ready;
				if (i < ready * 2 && ready / 3 != 0) {
					led_set_RGB(i, hsv2rgb(a));
					led_set_RGB(NUM_PIXELS - i - 1, hsv2rgb(a));
				} else {
					led_set_RGB(i, (RGB_t ) { 3, 0, 2 });
					led_set_RGB(NUM_PIXELS - i - 1, (RGB_t ) { 3, 0, 2 });
				}
			}
			led_render();
		}

		HAL_RTC_GetTime(&hrtc, &nowTime, RTC_FORMAT_BIN);
		display.displayClock(nowTime.Hours, nowTime.Minutes);
		if (nowTime.Seconds % 2) {
			display.point(1);
		} else {
			display.point(0);
		}

		if (BT_Set.isHolded()) {
			mode = SET_CLOCK;
			setsM = nowTime.Minutes;
			setsH = nowTime.Hours;
			sets = 0;
			on = true;
			timer = HAL_GetTick();
		}
		if ((BT_Plus.isHolded() || BT_Minus.isHolded()) && !alarmed) {
			mode = SET_ALARM;
			setingAlarm = 1;
			timer = HAL_GetTick();
			setsH = 0;
			setsM = 0;
			setsDelay = 0;
		}
		if (BT_Set.isClick()) {
			mode = CHOOSING_ALARM;
			setingAlarm = 1;
			timer = HAL_GetTick();
		}
		if (excited && nowTime.Hours == nowAlarm.Hours
				&& nowTime.Minutes == nowAlarm.Minutes && !alarmed) {
			alarmed = true;
			excited = false;
			player.status = PAUSE;
			player.volumeSet = 0;
		}
		if (alarmed
				&& (BT_Set.isClick() || BT_Plus.isClick() || BT_Minus.isClick())) {
			alarmed = false;
			excited = false;
			led_set_all_RGB((RGB_t ) { 0, 0, 0 });
			led_render();
			BT_Plus.resetStates();
			BT_Set.resetStates();
			BT_Minus.resetStates();
			changes = true;
		}

		if (alarmed && HAL_GetTick() - timer >= BLINK_TIMER) {
			if (blink) {
				led_set_all_RGB((RGB_t ) { 255, 255, 255 });
				blink = 0;
			} else {
				led_set_all_RGB((RGB_t ) { 0, 0, 0 });
				blink = 1;
			}
			timer = HAL_GetTick();
			led_render();
		}

		if (changes) {
			changes = false;
			for (uint8_t i = 0; i < BUFFSIZE - (BUFFSIZE - NUMBER_ALARMS);
					i++) {
				wdata[i] = (alarms[i].Hours << 16) + (alarms[i].Minutes << 8)
						+ alarms[i].Seconds;
			}
			wdata[BUFFSIZE - 2] = 0;
			if (excited) {
				wdata[BUFFSIZE - 2] += (1 << 16);
			}
			if (alarmed) {
				wdata[BUFFSIZE - 2] += (1 << 8);
			}
			wdata[BUFFSIZE - 2] += numberNowAlarm;
			write_to_flash(wdata);
		}

		while (mode == CHOOSING_ALARM) {
			BT_Plus.tick();
			BT_Set.tick();
			BT_Minus.tick();
			display.point(0);
			display.displayInt(setingAlarm);
			if (BT_Plus.isClick()) {
				timer = HAL_GetTick();
				if (setingAlarm >= NUMBER_ALARMS)
					setingAlarm = 0;
				else
					setingAlarm++;
			}
			if (BT_Minus.isClick()) {
				timer = HAL_GetTick();
				if (setingAlarm <= 0)
					setingAlarm = NUMBER_ALARMS;
				else
					setingAlarm--;
			}
			if (BT_Set.isClick()) {
				if (0 == setingAlarm) {
					excited = false;
					alarmed = false;
					changes = true;
					player.Volume(0);
					player.Pause();
					led_set_all_RGB((RGB_t ) { 0, 0, 0 });
					led_render();
				} else {
					nowAlarm = alarms[setingAlarm - 1];
					numberNowAlarm = setingAlarm - 1;
					excited = true;
					changes = true;
				}
				mode = NORMAL;
				BT_Plus.resetStates();
				BT_Set.resetStates();
				BT_Minus.resetStates();
			}
			if (HAL_GetTick() - timer >= USER_INACTIVITY_TIME) {
				mode = NORMAL;
			}
		}

		while (mode == SET_ALARM) {
			BT_Plus.tick();
			BT_Set.tick();
			BT_Minus.tick();
			if (BT_Plus.isClick()) {
				timer = HAL_GetTick();
				if (setingAlarm >= NUMBER_ALARMS)
					setingAlarm = 1;
				else
					setingAlarm++;
			}
			if (BT_Minus.isClick()) {
				timer = HAL_GetTick();
				if (setingAlarm <= 1)
					setingAlarm = NUMBER_ALARMS;
				else
					setingAlarm--;
			}
			display.point(0);
			display.displayInt(setingAlarm);
			if (BT_Set.isClick()) {
				sets = 0;
				setsH = alarms[setingAlarm - 1].Hours;
				setsM = alarms[setingAlarm - 1].Minutes;
				timer = HAL_GetTick();
				while (true) {
					BT_Plus.tick();
					BT_Set.tick();
					BT_Minus.tick();
					display.point(1);
					if (!on && HAL_GetTick() - setTimer >= 100) {
						on = 1;
						setTimer = HAL_GetTick();
					}
					if (on && HAL_GetTick() - setTimer >= 500) {
						on = 0;
						setTimer = HAL_GetTick();
					}
					if (on) {
						display.display(setsH / 10, setsH % 10, setsM / 10,
								setsM % 10);
					} else {
						if (sets == 0) {
							display.display(10, 10, setsM / 10, setsM % 10);
						}
						if (sets == 1) {
							display.display(setsH / 10, setsH % 10, 10, 10);
						}
					}
					if (sets == 0) {
						if (BT_Plus.isClick()) {
							timer = HAL_GetTick();
							if (setsH >= 23)
								setsH = 0;
							else
								setsH++;
						}
						if (BT_Plus.isHold()
								&& HAL_GetTick() - scrollTimer >= 50) {
							timer = HAL_GetTick();
							scrollTimer = HAL_GetTick();
							if (setsH >= 23)
								setsH = 0;
							else
								setsH++;
						}
						if (BT_Minus.isClick()) {
							timer = HAL_GetTick();
							if (setsH <= 0)
								setsH = 23;
							else
								setsH--;
						}
						if (BT_Minus.isHold()
								&& HAL_GetTick() - scrollTimer >= 50) {
							timer = HAL_GetTick();
							scrollTimer = HAL_GetTick();
							if (setsH <= 0)
								setsH = 23;
							else
								setsH--;
						}
					}
					if (sets == 1) {
						if (BT_Plus.isClick()) {
							timer = HAL_GetTick();
							if (setsM >= 59)
								setsM = 0;
							else
								setsM++;
						}
						if (BT_Plus.isHold()
								&& HAL_GetTick() - scrollTimer >= 100) {
							timer = HAL_GetTick();
							scrollTimer = HAL_GetTick();
							if (setsM >= 59)
								setsM = 0;
							else
								setsM++;
						}
						if (BT_Minus.isClick()) {
							timer = HAL_GetTick();
							if (setsM <= 0)
								setsM = 59;
							else
								setsM--;
						}
						if (BT_Minus.isHold()
								&& HAL_GetTick() - scrollTimer >= 100) {
							timer = HAL_GetTick();
							scrollTimer = HAL_GetTick();
							if (setsM <= 0)
								setsM = 59;
							else
								setsM--;
						}
					}
					if (sets == 2) {
						setsDelay = alarms[setingAlarm - 1].Seconds;
						while (true) {
							BT_Plus.tick();
							BT_Set.tick();
							BT_Minus.tick();
							display.point(0);
							if (BT_Plus.isClick()) {
								timer = HAL_GetTick();
								if (setsDelay >= 59) {
									setsDelay = 0;
								} else {
									setsDelay++;
								}
							}
							if (BT_Plus.isHold()
									&& HAL_GetTick() - scrollTimer >= 50) {
								timer = HAL_GetTick();
								scrollTimer = HAL_GetTick();
								if (setsDelay >= 59) {
									setsDelay = 0;
								} else {
									setsDelay++;
								}
							}
							if (BT_Minus.isClick()) {
								timer = HAL_GetTick();
								if (setsDelay <= 0) {
									setsDelay = 59;
								} else {
									setsDelay--;
								}
							}
							if (BT_Minus.isHold()
									&& HAL_GetTick() - scrollTimer >= 50) {
								timer = HAL_GetTick();
								scrollTimer = HAL_GetTick();
								if (setsDelay <= 0) {
									setsDelay = 59;
								} else {
									setsDelay--;
								}
							}
							if (BT_Set.isClick()) {
								timer = HAL_GetTick();
								if (alarms[setingAlarm - 1].Seconds
										!= setsDelay) {
									alarms[setingAlarm - 1].Seconds = setsDelay;
									changes = true;
								}
								sets++;
								break;
							}
							if (HAL_GetTick() - timer >= USER_INACTIVITY_TIME) {
								break;
							}
							display.displayInt(setsDelay);

						}
					}
					if (BT_Set.isHold() && sets < 2) {
						setsM = nowTime.Minutes;
						setsH = nowTime.Hours;
					}
					if (BT_Set.isClick()) {
						timer = HAL_GetTick();
						sets++;
					}

					if (sets >= 3) {
						if (alarms[setingAlarm - 1].Hours != setsH
								|| alarms[setingAlarm - 1].Minutes != setsM) {
							alarms[setingAlarm - 1].Hours = setsH;
							alarms[setingAlarm - 1].Minutes = setsM;
							if (setingAlarm - 1 == numberNowAlarm) {
								nowAlarm = alarms[setingAlarm - 1];
							}
							changes = true;
						}
						BT_Plus.resetStates();
						BT_Set.resetStates();
						BT_Minus.resetStates();
						mode = NORMAL;
						break;
					}
					if (HAL_GetTick() - timer >= USER_INACTIVITY_TIME) {
						mode = NORMAL;
						break;
					}

				}
			}
		}

		while (mode == SET_CLOCK) {
			BT_Plus.tick();
			BT_Set.tick();
			BT_Minus.tick();
			display.point(1);
			if (!on && HAL_GetTick() - setTimer >= 100) {
				on = 1;
				setTimer = HAL_GetTick();
			}
			if (on && HAL_GetTick() - setTimer >= 500) {
				on = 0;
				setTimer = HAL_GetTick();
			}
			if (on) {
				display.display(setsH / 10, setsH % 10, setsM / 10, setsM % 10);
			} else {
				if (sets == 0) {
					display.display(10, 10, setsM / 10, setsM % 10);
				}
				if (sets == 1) {
					display.display(setsH / 10, setsH % 10, 10, 10);
				}
			}
			if (sets == 0) {
				if (BT_Plus.isClick()) {
					timer = HAL_GetTick();
					if (setsH >= 23)
						setsH = 0;
					else
						setsH++;
				}
				if (BT_Plus.isHold() && HAL_GetTick() - scrollTimer >= 50) {
					timer = HAL_GetTick();
					scrollTimer = HAL_GetTick();
					if (setsH >= 23)
						setsH = 0;
					else
						setsH++;
				}
				if (BT_Minus.isClick()) {
					timer = HAL_GetTick();
					if (setsH <= 0)
						setsH = 23;
					else
						setsH--;
				}
				if (BT_Minus.isHold() && HAL_GetTick() - scrollTimer >= 50) {
					timer = HAL_GetTick();
					scrollTimer = HAL_GetTick();
					if (setsH <= 0)
						setsH = 23;
					else
						setsH--;
				}
			}
			if (sets == 1) {
				if (BT_Plus.isClick()) {
					timer = HAL_GetTick();
					if (setsM >= 59)
						setsM = 0;
					else
						setsM++;
				}
				if (BT_Plus.isHold() && HAL_GetTick() - scrollTimer >= 100) {
					timer = HAL_GetTick();
					scrollTimer = HAL_GetTick();
					if (setsM >= 59)
						setsM = 0;
					else
						setsM++;
				}
				if (BT_Minus.isClick()) {
					timer = HAL_GetTick();
					if (setsM <= 0)
						setsM = 59;
					else
						setsM--;
				}
				if (BT_Minus.isHold() && HAL_GetTick() - scrollTimer >= 100) {
					timer = HAL_GetTick();
					scrollTimer = HAL_GetTick();
					if (setsM <= 0)
						setsM = 59;
					else
						setsM--;
				}
			}
			if (BT_Set.isClick()) {
				timer = HAL_GetTick();
				sets++;
			}
			if (sets >= 2) {
				RTC_TimeTypeDef setingTime = { 0 };
				setingTime.Hours = setsH;
				setingTime.Minutes = setsM;
				setingTime.Seconds = 0;
				HAL_RTC_SetTime(&hrtc, &setingTime, RTC_FORMAT_BIN);
				BT_Plus.resetStates();
				BT_Set.resetStates();
				BT_Minus.resetStates();
				mode = NORMAL;
			}
			if (HAL_GetTick() - timer >= USER_INACTIVITY_TIME) {
				mode = NORMAL;
				break;
			}
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void) {

	/* USER CODE BEGIN CRC_Init 0 */

	/* USER CODE END CRC_Init 0 */

	/* USER CODE BEGIN CRC_Init 1 */

	/* USER CODE END CRC_Init 1 */
	hcrc.Instance = CRC;
	if (HAL_CRC_Init(&hcrc) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN CRC_Init 2 */

	/* USER CODE END CRC_Init 2 */

}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */
	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
	hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 0;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 60 - 1;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */
	HAL_TIM_MspPostInit(&htim1);

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 6000;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 4000;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */
	HAL_TIM_MspPostInit(&htim2);

}

/**
 * @brief TIM4 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM4_Init(void) {

	/* USER CODE BEGIN TIM4_Init 0 */

	/* USER CODE END TIM4_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM4_Init 1 */

	/* USER CODE END TIM4_Init 1 */
	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 71;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 65535;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM4_Init 2 */

	/* USER CODE END TIM4_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LED_Alarm_GPIO_Port, LED_Alarm_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB,
	Display_DIO_Pin | Display_CLK_Pin | Check_Speed_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LED_Alarm_Pin */
	GPIO_InitStruct.Pin = LED_Alarm_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_Alarm_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : Display_DIO_Pin Display_CLK_Pin Check_Speed_Pin */
	GPIO_InitStruct.Pin = Display_DIO_Pin | Display_CLK_Pin | Check_Speed_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : Button_1_Pin Button_2_Pin Button_3_Pin */
	GPIO_InitStruct.Pin = Button_1_Pin | Button_2_Pin | Button_3_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
RTC_TimeTypeDef remainingTime(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b) {
	RTC_TimeTypeDef out;
	uint16_t seconds;
	seconds = (a->Hours * 60 + a->Minutes) - (b->Hours * 60 + b->Minutes);
	out.Hours = seconds / 60;
	out.Minutes = seconds % 60;
	out.Seconds = 0;
	return out;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

