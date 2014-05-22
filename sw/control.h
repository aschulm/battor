#ifndef CONTROL_H
#define CONTROL_H

typedef enum CONTROL_TYPE_enum
{
	CONTROL_TYPE_AMPPOT_SET = 0,      // set the amp's potentiometer (gain)
	CONTROL_TYPE_FILPOT_SET,          // set the LPF's potentiometer (cutoff freq)
	CONTROL_TYPE_SAMPLE_TIMER_SET,    // set the sample timer (clock div and overflow) 
	CONTROL_TYPE_START_SAMPLING_UART, // start taking samples and sending them over the uart
	CONTROL_TYPE_START_SAMPLING_SD,   // start taking samples and storing them on the SD card
	CONTROL_TYPE_USB_POWER_SET        // change the state of the USB power
} CONTROL_TYPE_t;

struct control_message_
{
	uint8_t delim[2];
	uint8_t type;
	uint16_t value1;
	uint16_t value2;
} __attribute__((packed));
typedef struct control_message_ control_message;

void control(uint8_t type, uint16_t value1, uint16_t value2);

#endif
