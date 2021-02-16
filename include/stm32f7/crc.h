#pragma once

struct stm32_crc {
    uint32_t dr;        // 0x00
    uint32_t idr;       // 0x04
    uint32_t cr;        // 0x08
    uint32_t __res0;    // 0x0C
    uint32_t init;      // 0x10
    uint32_t pol;       // 0x14
};

static volatile struct stm32_crc *const crc = (struct stm32_crc *) 0x40023000;

#define CRC_CR_POLYSIZE_16      (1 << 3)
#define CRC_CR_RESET            (1 << 0)
