#ifndef BLINK_H
#define BLINK_H

#define BLINK_ON_TIME_MS 100

void blink_init(uint16_t interval_ms, uint8_t led);
void blink_ms_timer_update();
void blink_set_led(uint8_t led);
void blink_set_strobe_count(uint16_t count);
void blink_set_interval_ms(uint16_t interval_ms);

#endif
