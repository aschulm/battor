#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON_bm (1<<0)
#define BUTTON_UP 0
#define BUTTON_DOWN 1

#define BUTTON_HOLD_MS 2000

void button_init();

int button_self_test();
void button_ms_timer_update();

#endif
