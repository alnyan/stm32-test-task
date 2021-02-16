#include "cortex-m7/nvic.h"
#include "stm32f7/usart.h"
#include "stm32f7/gpio.h"
#include "stm32f7/crc.h"
#include "stm32f7/tim.h"
#include "stm32f7/rcc.h"

#include "leds.h"
#include "bits.h"
#include "util.h"

#include <stddef.h>

#define CLK_FREQ_HSI        16000000

// Limit: 255
#define BUFFER_SIZE     128

struct buffer {
    uint32_t count;
    uint8_t data[BUFFER_SIZE];
};
static struct buffer buf0, buf1;
static struct buffer *act = &buf0, *back_buf = &buf1;

static inline void store_sample(uint8_t v) {
    struct buffer *buf = act;
    if (act->count == BUFFER_SIZE) {
        act = back_buf;
        back_buf = buf;
        buf = act;
    }
    if (buf->count == BUFFER_SIZE) {
        led_set(LED_ERROR, 1);
        return;
    }

    buf->data[buf->count++] = v;
}

// Transmit a single byte of SLIP-like L2 payload
static void slip_tx(volatile struct stm32_usart *u, uint8_t b) {
    if (b == 0x55) {
        usart_tx(u, 0xAA);
        usart_tx(u, 0x00);
        return;
    }
    usart_tx(u, b);
    if (b == 0xAA) {
        usart_tx(u, 0x01);
    }
}

// Wrap and send sampled data using requested L2/L1 protocol stack
static void flush(volatile struct stm32_usart *u, struct buffer *buf) {
    uint32_t count = buf->count;
    usart_tx(u, 0x55);
    slip_tx(u, count);

    crc->cr |= 1;
    delay(100);
    *((uint8_t *) &crc->dr) = count & 0xFF;

    for (size_t i = 0; i < count; ++i) {
        slip_tx(u, buf->data[i]);
        *((uint8_t *) &crc->dr) = buf->data[i];
    }
    delay(100);
    uint16_t crc16 = *((uint16_t *) &crc->dr);
    slip_tx(u, crc16 >> 8);
    slip_tx(u, crc16 & 0xFF);
    buf->count = 0;
}

void irq_tim2(void) {
    tim2->sr &= ~(1 << 0);
    gpiob->odr ^= 1 << 4;

    uint8_t sample = (gpioc->idr >> 6) & 0xFF;
    store_sample(sample);
}

static void board_init(void) {
    // Basic clock init
    // Use HSI for AHB clock
    rcc->cr |= (1 << 0) /* RCC_CR_HSION */;
    rcc->cfgr &= ~(0xF << 4) /* Clear RCC_CFGR_HPRE */;
    // APB1 prescaler = 1
    rcc->cfgr &= ~(0x7 << 10);

    // Enable clocking for necessary peripherals
    rcc->ahb1enr |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_CRCEN;
    rcc->apb1enr |= RCC_APB1ENR_TIM2 | RCC_APB1ENR_USART2;

    BSET32_2(gpiob->moder, 4, GPIO_MODER_OUT);
    BSET32_2(gpiob->pupdr, 4, GPIO_PUPDR_PULL_DOWN);
    gpiob->otyper &= ~(1 << 4);

    // PC6  pin0
    // PC7  pin1
    // PC8  pin2
    // PC9  pin3
    // PC10 pin4
    // PC11 pin5
    // PC12 pin6
    // PC13 pin7 (also push button on my board)
    BSET32_2(gpioc->moder, 6, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 6, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 7, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 7, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 8, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 8, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 9, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 9, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 10, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 10, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 11, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 11, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 12, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 12, GPIO_PUPDR_PULL_DOWN);
    BSET32_2(gpioc->moder, 13, GPIO_MODER_IN);
    BSET32_2(gpioc->pupdr, 13, GPIO_PUPDR_PULL_DOWN);

    // PD5 - USART2_TX
    BSET32_2(gpiod->moder, 5, GPIO_MODER_ALT);
    BSET32_4(gpiod->afrl, 5, 7);

    // Enable transmit at 38400baud
    usart2->cr1 |= USART_CR1_TE;
    usart2->brr = CLK_FREQ_HSI / 38400;
    usart2->cr1 |= USART_CR1_UE;

    // CRC-16-IBM: init=0xFFFF, pol=0xA001
    crc->cr |= CRC_CR_POLYSIZE_16;
    crc->init = 0xFFFF;
    crc->pol = 0xA001;
    crc->cr |= CRC_CR_RESET;

    // 1kHz: base freq 1MHz / reload value of 1000
    tim2->psc = CLK_FREQ_HSI / 1000000 - 1;
    tim2->arr = 1000 - 1;
    // Only send updates on overflow
    tim2->cr1 |= TIM_CR2_URS;
    // Enable update interrupt
    tim2->dier |= TIM_DIER_UIE;
    // Enable counter
    tim2->cr1 |= TIM_CR1_CEN;

    nvic_irq_enable(NVIC_IRQ_TIM2);

    leds_init();
}

_Noreturn void main(void) {
    board_init();

    while (1) {
        asm volatile ("wfe");
        if (back_buf->count == BUFFER_SIZE) {
            led_set(LED_ACTIVITY, 1);
            flush(usart2, back_buf);
            led_set(LED_ACTIVITY, 0);
        }
    }
}
