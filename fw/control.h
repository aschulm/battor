#ifndef CONTROL_H
#define CONTROL_H

#define CONTROL_DELIM0 0xBA
#define CONTROL_DELIM1 0x77

typedef enum CONTROL_TYPE_enum
{
	CONTROL_TYPE_INIT = 0,
	CONTROL_TYPE_RESET,               // reset the MCU
	CONTROL_TYPE_SELF_TEST,           // run a self test
	CONTROL_TYPE_READ_EEPROM,         // read the EEPROM
	CONTROL_TYPE_GAIN_SET,            // set the current measurement gain
	CONTROL_TYPE_START_SAMPLING_UART, // start taking samples and sending them over the uart
	CONTROL_TYPE_START_SAMPLING_SD,   // start taking samples and storing them on the SD card
	CONTROL_TYPE_READ_SD_UART,        // read a file from the SD card over the UART
	CONTROL_TYPE_GET_SAMPLE_COUNT,    // read the number of samples collected for sync
	CONTROL_TYPE_GET_GIT_HASH,        // read the GIT hash of the firmware
	CONTROL_TYPE_GET_MODE_PORTABLE,   // read if the BattOr is portable or not
	CONTROL_TYPE_SET_RTC,             // write the RTC seconds
} CONTROL_TYPE_t;

typedef struct control_message_ 
{
	uint8_t	type;
	uint16_t value1;
	uint16_t value2;
} control_message;

typedef enum CONTROL_MODE_enum
{
	CONTROL_MODE_USB_IDLE = 1,
	CONTROL_MODE_PORT_IDLE,
	CONTROL_MODE_USB_STORE,
	CONTROL_MODE_PORT_STORE,
	CONTROL_MODE_STREAM,
} CONTROL_MODE_t;

extern uint8_t g_control_mode;
extern uint8_t g_control_gain;

void control_got_uart_bytes();
int8_t control_run_message(control_message* m);
void control_button_press();
void control_button_hold();

#endif
