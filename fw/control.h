#ifndef CONTROL_H
#define CONTROL_H

#define CONTROL_DELIM0 0xBA
#define CONTROL_DELIM1 0x77
#define CONTROL_ACK 0x01

typedef enum CONTROL_TYPE_enum
{
	CONTROL_TYPE_AMPPOT_SET = 0,      // set the amp's potentiometer (gain)
	CONTROL_TYPE_AMPPOT_GET,          // get the amp's potentiometer (gain)
	CONTROL_TYPE_FILPOT_SET,          // set the LPF's potentiometer (cutoff freq)
	CONTROL_TYPE_FILPOT_GET,          // get the LPF's potentiometer (cutoff freq)
	CONTROL_TYPE_SAMPLE_TIMER_SET,    // set the sample timer (clock div and overflow) 
	CONTROL_TYPE_SAMPLE_TIMER_GET,    // get the sample timer (clock div and overflow) 
	CONTROL_TYPE_START_SAMPLING_UART, // start taking samples and sending them over the uart
	CONTROL_TYPE_START_SAMPLING_SD,   // start taking samples and storing them on the SD card
	CONTROL_TYPE_USB_POWER_SET,       // change the state of the USB power
	CONTROL_TYPE_START_REC_CONTROL,   // init recording the control messages to the SD card
	CONTROL_TYPE_END_REC_CONTROL,     // write the control messages to the SD card
	CONTROL_TYPE_READ_FILE            // read a file from the SD card
} CONTROL_TYPE_t;

typedef struct control_message_ 
{
	uint8_t delim[2];
	uint8_t	type;
	uint16_t value1;
	uint16_t value2;
} control_message;

typedef enum CONTROL_MODE_enum
{
	CONTROL_MODE_STREAM = 1,
	CONTROL_MODE_STORE,
	CONTROL_MODE_REC_CONTROL
} CONTROL_MODE_t;

extern uint8_t g_control_mode;

void control_got_uart_bytes();
void control_run_message(control_message* m);

#endif
