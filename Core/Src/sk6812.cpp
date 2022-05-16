// Peripheral usage
#include "stm32f1xx_hal.h"
extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch1;

#include "sk6812.h"

const uint8_t max_whiteness = 15;
const uint8_t max_value = 15;

const uint8_t sixth_hue = 16;
const uint8_t third_hue = sixth_hue * 2;
const uint8_t half_hue = sixth_hue * 3;
const uint8_t two_thirds_hue = sixth_hue * 4;
const uint8_t five_sixths_hue = sixth_hue * 5;
const uint8_t full_hue = sixth_hue * 6;

inline RGB_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (RGB_t) {r, g, b};
}

inline HSV_t hsv(uint8_t h, uint8_t s, uint8_t v) {
    return (HSV_t) {h, s, v};
}

const RGB_t black = {0, 0, 0};

RGB_t hsv2rgb(HSV_t hsv) {
    if (hsv.v == 0) return black;

    uint8_t high = hsv.v * max_whiteness;//channel with max value
    if (hsv.s == 0) return rgb(high, high, high);

    uint8_t W = max_whiteness - hsv.s;
    uint8_t low = hsv.v * W;//channel with min value
    uint8_t rising = low;
    uint8_t falling = high;

    uint8_t h_after_sixth = hsv.h % sixth_hue;
    if (h_after_sixth > 0) {//not at primary color? ok, h_after_sixth = 1..sixth_hue - 1
        uint8_t z = hsv.s * uint8_t(hsv.v * h_after_sixth) / sixth_hue;
        rising += z;
        falling -= z + 1;//it's never 255, so ok
    }

    uint8_t H = hsv.h;
    while (H >= full_hue) H -= full_hue;

    if (H < sixth_hue) return rgb(high, rising, low);
    if (H < third_hue) return rgb(falling, high, low);
    if (H < half_hue) return rgb(low, high, rising);
    if (H < two_thirds_hue) return rgb(low, falling, high);
    if (H < five_sixths_hue) return rgb(rising, low, high);
    return rgb(high, low, falling);
}

// LED color buffer
uint8_t rgb_arr[NUM_BYTES] = { 0 };

// LED write buffer
uint8_t wr_buf[WR_BUF_LEN] = { 0 };
uint_fast8_t wr_buf_p = 0;

static inline uint8_t scale8(uint8_t x, uint8_t scale) {
	return ((uint16_t) x * scale) >> 8;
}

// Set a single color (RGB) to index
void led_set_RGB(uint8_t index, RGB_t input) {
#if (NUM_BPP == 4) // SK6812
  rgb_arr[4 * index] = scale8(input.g, 0xB0); // g;
  rgb_arr[4 * index + 1] = input.r;
  rgb_arr[4 * index + 2] = scale8(input.b, 0xF0); // b;
  rgb_arr[4 * index + 3] = 0;
#else // WS2812B
	rgb_arr[3 * index] = scale8(input.g, 0xB0); // g;
	rgb_arr[3 * index + 1] = input.r;
	rgb_arr[3 * index + 2] = scale8(input.b, 0xF0); // b;
#endif // End SK6812 WS2812B case differentiation
}

// Set a single color (RGBW) to index
void led_set_RGBW(uint8_t index, RGB_t input, uint8_t w) {
	led_set_RGB(index, input);
#if (NUM_BPP == 4) // SK6812
  rgb_arr[4 * index + 3] = w;
#endif // End SK6812 WS2812B case differentiation
}

// Set all colors to RGB
void led_set_all_RGB(RGB_t input) {
	for (uint_fast8_t i = 0; i < NUM_PIXELS; ++i)
		led_set_RGB(i, input);
}

// Set all colors to RGBW
void led_set_all_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
	for (uint_fast8_t i = 0; i < NUM_PIXELS; ++i)
		led_set_RGBW(i, r, g, b, w);
}

// Shuttle the data to the LEDs!
void led_render() {
	if (wr_buf_p != 0 || hdma_tim1_ch1.State != HAL_DMA_STATE_READY) {
		// Ongoing transfer, cancel!
		for (uint8_t i = 0; i < WR_BUF_LEN; ++i)
			wr_buf[i] = 0;
		wr_buf_p = 0;
		HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
		return;
	}
	// Ooh boi the first data buffer half (and the second!)
#if (NUM_BPP == 4) // SK6812
  for(uint_fast8_t i = 0; i < 8; ++i) {
    wr_buf[i     ] = PWM_LO << (((rgb_arr[0] << i) & 0x80) > 0);
    wr_buf[i +  8] = PWM_LO << (((rgb_arr[1] << i) & 0x80) > 0);
    wr_buf[i + 16] = PWM_LO << (((rgb_arr[2] << i) & 0x80) > 0);
    wr_buf[i + 24] = PWM_LO << (((rgb_arr[3] << i) & 0x80) > 0);
    wr_buf[i + 32] = PWM_LO << (((rgb_arr[4] << i) & 0x80) > 0);
    wr_buf[i + 40] = PWM_LO << (((rgb_arr[5] << i) & 0x80) > 0);
    wr_buf[i + 48] = PWM_LO << (((rgb_arr[6] << i) & 0x80) > 0);
    wr_buf[i + 56] = PWM_LO << (((rgb_arr[7] << i) & 0x80) > 0);
  }
#else // WS2812B
	for (uint_fast8_t i = 0; i < 8; ++i) {
		wr_buf[i] = PWM_LO << (((rgb_arr[0] << i) & 0x80) > 0);
		wr_buf[i + 8] = PWM_LO << (((rgb_arr[1] << i) & 0x80) > 0);
		wr_buf[i + 16] = PWM_LO << (((rgb_arr[2] << i) & 0x80) > 0);
		wr_buf[i + 24] = PWM_LO << (((rgb_arr[3] << i) & 0x80) > 0);
		wr_buf[i + 32] = PWM_LO << (((rgb_arr[4] << i) & 0x80) > 0);
		wr_buf[i + 40] = PWM_LO << (((rgb_arr[5] << i) & 0x80) > 0);
	}
#endif // End SK6812 WS2812B case differentiation

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*) wr_buf,
			WR_BUF_LEN);
	wr_buf_p = 2; // Since we're ready for the next buffer
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	// DMA buffer set from LED(wr_buf_p) to LED(wr_buf_p + 1)
	if (wr_buf_p < NUM_PIXELS) {
		// We're in. Fill the even buffer
#if (NUM_BPP == 4) // SK6812
    for(uint_fast8_t i = 0; i < 8; ++i) {
      wr_buf[i     ] = PWM_LO << (((rgb_arr[4 * wr_buf_p    ] << i) & 0x80) > 0);
      wr_buf[i +  8] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 1] << i) & 0x80) > 0);
      wr_buf[i + 16] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 2] << i) & 0x80) > 0);
      wr_buf[i + 24] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 3] << i) & 0x80) > 0);
    }
#else // WS2812B
		for (uint_fast8_t i = 0; i < 8; ++i) {
			wr_buf[i] = PWM_LO << (((rgb_arr[3 * wr_buf_p] << i) & 0x80) > 0);
			wr_buf[i + 8] = PWM_LO
					<< (((rgb_arr[3 * wr_buf_p + 1] << i) & 0x80) > 0);
			wr_buf[i + 16] = PWM_LO
					<< (((rgb_arr[3 * wr_buf_p + 2] << i) & 0x80) > 0);
		}
#endif // End SK6812 WS2812B case differentiation
		wr_buf_p++;
	} else if (wr_buf_p < NUM_PIXELS + 2) {
		// Last two transfers are resets. SK6812: 64 * 1.25 us = 80 us == good enough reset
		//                               WS2812B: 48 * 1.25 us = 60 us == good enough reset
		// First half reset zero fill
		for (uint8_t i = 0; i < WR_BUF_LEN / 2; ++i)
			wr_buf[i] = 0;
		wr_buf_p++;
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	// DMA buffer set from LED(wr_buf_p) to LED(wr_buf_p + 1)
	if (wr_buf_p < NUM_PIXELS) {
		// We're in. Fill the odd buffer
#if (NUM_BPP == 4) // SK6812
    for(uint_fast8_t i = 0; i < 8; ++i) {
      wr_buf[i + 32] = PWM_LO << (((rgb_arr[4 * wr_buf_p    ] << i) & 0x80) > 0);
      wr_buf[i + 40] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 1] << i) & 0x80) > 0);
      wr_buf[i + 48] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 2] << i) & 0x80) > 0);
      wr_buf[i + 56] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 3] << i) & 0x80) > 0);
    }
#else // WS2812B
		for (uint_fast8_t i = 0; i < 8; ++i) {
			wr_buf[i + 24] = PWM_LO
					<< (((rgb_arr[3 * wr_buf_p] << i) & 0x80) > 0);
			wr_buf[i + 32] = PWM_LO
					<< (((rgb_arr[3 * wr_buf_p + 1] << i) & 0x80) > 0);
			wr_buf[i + 40] = PWM_LO
					<< (((rgb_arr[3 * wr_buf_p + 2] << i) & 0x80) > 0);
		}
#endif // End SK6812 WS2812B case differentiation
		wr_buf_p++;
	} else if (wr_buf_p < NUM_PIXELS + 2) {
		// Second half reset zero fill
		for (uint8_t i = WR_BUF_LEN / 2; i < WR_BUF_LEN; ++i)
			wr_buf[i] = 0;
		++wr_buf_p;
	} else {
		// We're done. Lean back and until next time!
		wr_buf_p = 0;
		HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	}
}
