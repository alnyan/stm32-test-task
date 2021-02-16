#pragma once

// PB1/PB7/PB14 respectively
#define LED_ACTIVITY    (1 << 0)
#define LED_BLINK       (1 << 7)
#define LED_ERROR       (1 << 14)

void leds_init(void);
void led_set(int which, int state);
void led_toggle(int which);
