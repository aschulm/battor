#ifndef LED_H
#define LED_H

#define LED_RED_bm (1<<2)
#define LED_YELLOW_bm (1<<1)
#define LED_GREEN_bm (1<<0)

void led_init();
void led_on(char led);
void led_off(char led);
void led_toggle(char led);
int led_self_test();

#endif
