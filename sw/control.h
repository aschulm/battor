#ifndef CONTROL_H
#define CONTROL_H

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
	CONTROL_TYPE_GET_RTC,             // read the RTC for the specified file
} CONTROL_TYPE_t;

struct control_message_
{
	uint8_t type;
	uint16_t value1;
	uint16_t value2;
} __attribute__((packed));
typedef struct control_message_ control_message;

struct control_ack_
{
	uint8_t type;
	uint8_t value;
} __attribute__((packed));
typedef struct control_ack_ control_ack;

int control(uint8_t type, uint16_t value1, uint16_t value2, uint8_t ack);

#endif
