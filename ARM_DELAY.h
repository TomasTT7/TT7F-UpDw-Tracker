
#ifndef ARM_DELAY_H
#define ARM_DELAY_H

#include "stdint.h"

static volatile uint32_t timestamp = 0;

// Functions
void SysTick_delay_init(void);
void SysTick_delay_ms(uint32_t ms);
void SysTick_Handler(void);

#endif // ARM_DELAY_H_