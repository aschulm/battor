#ifndef CONTROL_H
#define CONTROL_H

#define CONTROL_DELIM0 0xBA
#define CONTROL_DELIM1 0x77
#define CONTROL_ACK 0x01

typedef enum CONTROL_TYPE_enum
{
	CONTROL_TYPE_INIT = 0,
	CONTROL_TYPE_AMPPOT_SET,          // set the amp's potentiometer (gain)
	CONTROL_TYPE_FILPOT_SET,          // set the LPF's potentiometer (cutoff freq)
	CONTROL_TYPE_SAMPLE_TIMER_SET,    // set the sample timer (clock div and overflow) 
	CONTROL_TYPE_START_SAMPLING_UART, // start taking samples and sending them over the uart
	CONTROL_TYPE_START_SAMPLING_SD,   // start taking samples and storing them on the SD card
	CONTROL_TYPE_START_REC_CONTROL,   // init recording the control messages to the SD card
	CONTROL_TYPE_END_REC_CONTROL,     // write the control messages to the SD card
	CONTROL_TYPE_READ_FILE,           // read a file from the SD card
	CONTROL_TYPE_READ_READY,          // ready to start receiving next round of samples
	CONTROL_TYPE_RESET,               // reset the MCU
	CONTROL_TYPE_READ_EEPROM          // read the EEPROM
} CONTROL_TYPE_t;

typedef struct control_message_ 
{
	uint8_t	type;
	uint16_t value1;
	uint16_t value2;
} control_message;

typedef enum CONTROL_MODE_enum
{
	CONTROL_MODE_IDLE,
	CONTROL_MODE_STREAM,
	CONTROL_MODE_STORE,
	CONTROL_MODE_REC_CONTROL,
	CONTROL_MODE_READ_FILE
} CONTROL_MODE_t;

extern uint8_t g_control_mode;
extern uint8_t g_control_calibrated;
extern uint8_t g_control_read_ready;

void control_got_uart_bytes();
int8_t control_run_message(control_message* m);

#endif
