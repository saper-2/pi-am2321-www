
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include "am2321.h"

// after sending address of AM2321 and before geting ACK you have to wait for at least 800us for AM2321 to wakeup
// So we will wait time in unit of I2C_HDELAY , so if I2C_HDELAY=50 then we need to wait: 16*50=800 [us]

uint16_t crc16_update(uint16_t crc, uint8_t a) {
	int i;

	crc ^= a;
	for (i = 0; i < 8; ++i)
	{
	    if (crc & 1)
		crc = (crc >> 1) ^ 0xA001;
	    else
		crc = (crc >> 1);
	}

	return crc;
} 

// delay
long time2us(struct timeval t) { return (t.tv_sec * 1000000) + t.tv_usec; }

void delay1us(long us) {
	struct timeval cr;
	struct timeval sta;

	gettimeofday(&sta, NULL);
	do {
		gettimeofday(&cr, NULL);
	} while((time2us(cr) - time2us(sta)) < us );
}

// internal function
int am2321_wakeup(char* i2cbus) {

	int f, r, retires=0;
	f = open(i2cbus, O_RDWR); // i2c_start();
	if (f < 0) {
		fprintf(stderr, "Unable to open i2c bus for write. (am2321_wakeup)\n");
		return AM2321_ERROR_BUS_OPEN;
	}
	r = ioctl(f, I2C_SLAVE, AM2321_I2C_ADDR);
	if (r < 0) {
		fprintf(stderr, "Unable to set address of AM2321 on i2c bus. (am2321_wakeup)\n");
		return AM2321_ERROR_BUS_CONFIG;
	}
	
	retires=0;
am2321_wakeup_write1:
	r = write(f, NULL, 0); // res = i2c_send(AM2321_I2C_ADDR);
	if ((r < 0) && ((retires++) < 6)) {
		delay1us(10000);// delay1ms(3);
		fprintf(stderr, "Unable to write data to device on i2c bus. Retry %i. (am2321_wakeup)\n",retires);
		goto am2321_wakeup_write1;
	}
	// end goto
	if (r < 0) {
		fprintf(stderr, "Unable to write data to device on i2c bus. (am2321_wakeup)\n");
		return AM2321_ERROR_BUS_WRITE;
	}
	close(f); //i2c_stop();
	
	delay1us(10000);// delay1ms(3);
	
	return AM2321_OK;
}

// data must be at least 10 bytes long
int am2321_read_info(uint8_t *data, char* i2cbus) {
	int f, r, retries=0;
	uint16_t crc=0xffff, crc2 = 0xffff;
	
	uint8_t cmd[24];
	/*if (sizeof(data) < 10) {
		fprintf(stderr, "Array size for result is too small.\n");
		return AM2321_ERROR;
	}*/
	fprintf(stderr, "AM2321 read_info begin...\n");
	
	r = am2321_wakeup(i2cbus);
	if (r != AM2321_OK) {
		return r;
	}
	
	f = open(i2cbus, O_RDWR); // i2c_start();
	if (f < 0) {
		fprintf(stderr, "Unable to open i2c bus for write. (am2321_read_info)\n");
		return AM2321_ERROR_BUS_OPEN;
	}
	r = ioctl(f, I2C_SLAVE, AM2321_I2C_ADDR);
	if (r < 0) {
		fprintf(stderr, "Unable to set address of AM2321 on i2c bus. (am2321_read_info)\n");
		return AM2321_ERROR_BUS_CONFIG;
	}
	
	// prepare command
	cmd[0] = 0x03; // function code: read multiple registers
	cmd[1] = 0x08; // start addres
	cmd[2] = 0x08; // numbers of registers to read
	
	retries=0;
am2321_read_info_write1:
	r = write(f, cmd, 3); //send data to am2321
	if ((r < 0) && ((retries++) < 6) ) {
		delay1us(10000);// delay1ms(3); // give AM2321 to prepare data
		fprintf(stderr, "am2321_read_info_write retry %i...\n",retries);
		goto am2321_read_info_write1;
	}
	// end of goto
	if (r < 0) {
		fprintf(stderr, "Unable to write data to AM2321 on i2c bus. (am2321_read_info)\n");
		return AM2321_ERROR_BUS_WRITE;
	}
	delay1us(10000);// delay1ms(3); // give AM2321 to prepare data
	
	// read 10 bytes:
	// 2 bytes header (cmd, len), 8 bytes data, 2 bytes crc 16
	memset(cmd,0x00, sizeof(cmd));
	r = read(f, cmd, 12);
	if (r < 0) {
		fprintf(stderr, "Unable to read data from AM2321 on i2c bus. (am2321_read_info)\n");
		return AM2321_ERROR_BUS_READ;
	}
	close(f); //i2c_stop();
	
	// get crc from data
	crc = cmd[10] | (((uint16_t)cmd[11])<<8);
	// calc crc2
	for(int i=0;i<10;i++) crc2 = crc16_update(crc2, cmd[i]);
	
	if (crc != crc2) {
		fprintf(stderr,"Calculated crc16 mismatch received crc16 from AM2321. Got=0x%04X Calc=0x%04X\n",crc,crc2);
		return AM2321_ERROR_CRC;
	}
	
	memset(data,0,8);
	for(int i=2,j=0;i<10;i++,j++) {
		data[j] = cmd[i];
	}
	
	return AM2321_OK;
}

int am2321_read_raw(struct AM2321_STRUCT *raw, char* i2cbus) {
	int r, f, retries=0;
	uint8_t cmd[24];
	uint16_t crc=0xffff, crc2=0xffff;
	
	fprintf(stderr, "AM2321 read_raw begin...\n");
	r = am2321_wakeup(i2cbus);
	if (r != AM2321_OK) {
		//return r;
		fprintf(stderr, "AM2321 wake-up failed. (am2321_read_raw)\n");
	}
	delay1us(1000); // 1ms
	
	f = open(i2cbus, O_RDWR); // i2c_start();
	if (f < 0) {
		fprintf(stderr, "Unable to open i2c bus for write. (am2321_read_raw)\n");
		return AM2321_ERROR_BUS_OPEN;
	}
	r = ioctl(f, I2C_SLAVE, AM2321_I2C_ADDR);
	if (r < 0) {
		fprintf(stderr, "Unable to set address of AM2321 on i2c bus. (am2321_read_raw)\n");
		return AM2321_ERROR_BUS_CONFIG;
	}
	
	// prepare command
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x03; // function code: read multiple registers
	cmd[1] = 0x00; // start addres
	cmd[2] = 0x04; // numbers of registers to read
am2321_read_raw_write1:
	r = write(f, cmd, 3); //send data to am2321
	if ((r < 0) && ((retries++) < 6) ) {
		delay1us(10000);// delay1ms(3); // give AM2321 to prepare data
		fprintf(stderr, "am2321_read_raw_write retry %i...\n",retries);
		goto am2321_read_raw_write1;
	}
	// end of goto
	if (r < 0) {
		fprintf(stderr, "Unable to write data to AM2321 on i2c bus. (am2321_read_raw)\n");
		return AM2321_ERROR_BUS_WRITE;
	}
	delay1us(10000);// delay1ms(3); // give AM2321 to prepare data	
	
	// read 10 bytes:
	// 2 bytes header (cmd, len), 8 bytes data, 2 bytes crc 16
	memset(cmd,0x00, sizeof(cmd));
	r = read(f, cmd, 8);
	if (r < 0) {
		fprintf(stderr, "Unable to read data from AM2321 on i2c bus. (am2321_read_raw)\n");
		return AM2321_ERROR_BUS_READ;
	}
	close(f); //i2c_stop();
	
	
	fprintf(stderr,"Read data: ");
	for(int i=0;i<8;i++) fprintf(stderr, "%02X ",cmd[i]);
	fprintf(stderr,"\n");
	
	
	// get crc from data
	crc = cmd[6] | (((uint16_t)cmd[7])<<8);
	// calc crc2
	for(int i=0;i<6;i++) crc2 = crc16_update(crc2, cmd[i]);
	
	if (crc != crc2) {
		fprintf(stderr,"Calculated crc16 mismatch received crc16 from AM2321. Got=0x%04X Calc=0x%04X\n",crc,crc2);
		return AM2321_ERROR_CRC;
	}
	
	raw->humi = (((uint16_t)cmd[2])<<8) | cmd[3];
	raw->temp = (((int16_t)cmd[4])<<8) | cmd[5];
	raw->state = AM2321_DATA_HUMAN;
	
	return AM2321_OK;
}

