
#include <libopencm3/stm32/rcc.h>
#include "delay.h"

extern uint32_t rcc_ahb_frequency;

uint32_t ticks_per_us;

void delay_init(void)
{
  COREDEBUG_DEMCR |= COREDEBUG_TRCENA;
  DWT_CYCCNT = 0;
  DWT_CTRL |= DWT_CYCCNTENA;
  ticks_per_us = (rcc_ahb_frequency / 1000000u);
}

