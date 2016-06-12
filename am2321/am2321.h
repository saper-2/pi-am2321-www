#ifndef _AM2321_H_
#define _AM2321_H_

#define AM2321_I2C_ADDR (0xB8 >> 1) // Linux uses 7bit addressing as default (doesn't include r/w bit into address...)

#define AM2321_WAKEUP_UNIT_COUNT 16 
#define AM2321_OK 0
#define AM2321_ERROR 1
#define AM2321_ERROR_CRC 2
#define AM2321_ERROR_RAW 3
#define AM2321_ERROR_BUS_OPEN 4
#define AM2321_ERROR_BUS_CONFIG 5
#define AM2321_ERROR_BUS_WRITE 6
#define AM2321_ERROR_BUS_READ 7
#define AM2321_DATA_NONE 0
#define AM2321_DATA_RAW 1
#define AM2321_DATA_HUMAN 2
//#define AM2321_
//#define AM2321_

struct AM2321_STRUCT {
	uint16_t humi;
	int16_t  temp;
	uint8_t  state;
} ;

typedef struct AM2321_STRUCT am2321_t;

int am2321_read_info(uint8_t *data, char* i2cbus);
int am2321_read_raw(struct AM2321_STRUCT *raw, char* i2cbus);

uint16_t crc16_update(uint16_t crc, uint8_t a);
void delay1us(long us);

//extern struct AM2321_STRUCT am2321_data;

#endif