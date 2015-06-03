#ifndef GPIO_H
#define GPIO_H

inline void gpio_on(PORT_t* port, uint8_t pin);
inline void gpio_off(PORT_t* port, uint8_t pin);
inline void gpio_toggle(PORT_t* port, uint8_t pin);
inline uint8_t gpio_read(PORT_t* port, uint8_t pin_p);

#endif
