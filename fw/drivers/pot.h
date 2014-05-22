#ifndef POT_H
#define POT_H

#define POT_AMP_CS_PIN_gm (1<<3)
#define POT_FIL_CS_PIN_gm (1<<4)
#define MAX_WIPERPOS 1023
#define MIN_WIPERPOS 0

void pot_init();
uint16_t pot_wiperpos_get(uint8_t pot_cs_pin);
void pot_wiperpos_set(uint8_t pot_cs_pin, uint16_t pos);

#endif
