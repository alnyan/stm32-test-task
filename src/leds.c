#include "stm32f7/gpio.h"

#include "bits.h"
#include "leds.h"

void led_toggle(int which) {
    gpiob->odr ^= which;
}

void led_set(int which, int state) {
    if (state) {
        gpiob->odr |= which;
    } else {
        gpiob->odr &= ~which;
    }
}

void leds_init(void) {
    BSET32_2(gpiob->moder, 0, GPIO_MODER_OUT);
    BSET32_2(gpiob->pupdr, 0, GPIO_PUPDR_PULL_DOWN);
    gpiob->otyper &= ~(1 << 0);

    BSET32_2(gpiob->moder, 7, GPIO_MODER_OUT);
    BSET32_2(gpiob->pupdr, 7, GPIO_PUPDR_PULL_DOWN);
    gpiob->otyper &= ~(1 << 7);

    BSET32_2(gpiob->moder, 14, GPIO_MODER_OUT);
    BSET32_2(gpiob->pupdr, 14, GPIO_PUPDR_PULL_DOWN);
    gpiob->otyper &= ~(1 << 14);
}

