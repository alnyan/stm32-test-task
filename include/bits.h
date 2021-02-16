#pragma once

#define BSET32_2(reg, ind, val) \
    (reg) &= ~(0x3U << ((ind) * 2)); \
    (reg) |= (val) << ((ind) * 2)

#define BSET32_4(reg, ind, val) \
    (reg) &= ~(0xFU << ((ind) * 4)); \
    (reg) |= (val) << ((ind) * 4)
