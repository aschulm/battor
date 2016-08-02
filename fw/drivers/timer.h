#ifndef TIMER_H
#define TIMER_H

extern volatile uint16_t g_timer_ms_ticks;

void timer_init(TC0_t* timer, uint8_t int_level);
void timer_reset(TC0_t* timer);
void timer_set(TC0_t* timer, uint8_t prescaler, uint16_t period);
void timer_sleep_ms(uint16_t ms);
uint16_t timer_elapsed_ms(uint16_t prev, uint16_t curr);

#endif
