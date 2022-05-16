#ifndef _LED_DRIVER_SK6812
#define _LED_DRIVER_SK6812

#include <stdint.h>

#define PWM_HI (38)
#define PWM_LO (19)
// LED parameters
#define NUM_BPP (3) // WS2812B
//#define NUM_BPP (4) // SK6812
#define NUM_PIXELS (60)
#define NUM_BYTES (NUM_BPP * NUM_PIXELS)
#define WR_BUF_LEN (NUM_BPP * 8 * 2)

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_t;

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} HSV_t;

RGB_t hsv2rgb(HSV_t hsv);

void led_set_RGB(uint8_t index, RGB_t);
void led_set_RGBW(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_set_all_RGB(RGB_t);
void led_set_all_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_render();

#endif
