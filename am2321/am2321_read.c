
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include "am2321.h"

//#define DEBUG

char i2c_bus[128];
int raw_mode = 0;
int val_mode = 0;
int info_mode = 0;

void print_help(void) {
	fprintf(stderr, "Usage: am2321-read [-b i2c_bus_num] [-h] [-bf path_to_bus] [-raw] [-val] [-info]\n");
	fprintf(stderr, "Parameters:\n");
	fprintf(stderr, "  -b i2c_bus_num  - specify i2c bus by it's number (/dev/i2c-xyz)\n");
	fprintf(stderr, "  -bf path_to_bus - specify i2c bus device by full path (e.g.: /dev/i2c-1)\n");
	fprintf(stderr, "  -raw            - read values from AM2321 will be printed in hex format without calculations\n");
	fprintf(stderr, "  -val            - read values from AM2321\n");
	fprintf(stderr, "  -info           - read information from AM2321 (registers 0x08..0x0F)\n");
	fprintf(stderr, "  -h              - show this help\n");
	fprintf(stderr, "\n\n");
	fprintf(stderr, "** Output formats **\n");
	fprintf(stderr, "Depending on used options there can be at most two lines returned.\n");
	fprintf(stderr, "Line with sensor value data start with 'v' , while line with sensor info start with 'i'. Line ends with 0x10 (ascii:LF).\n");
	fprintf(stderr, "One line will be returned with used options: raw and/or val , or: info (without raw and/or val)\n");
	fprintf(stderr, "Explanations: rectangle parenthesis [] - defines dynamic value field,  before colon (:) is field name, after colon is data type.\n");
	fprintf(stderr, "If data type is followed by number in round parenthesis () then, the number defines fixed length of field.\n");
	fprintf(stderr, "Below is description of line formats: \n");
	fprintf(stderr, "-raw \n");
	fprintf(stderr, "     v:[unix_ts:uint];[mode:char];[result:int];[humi:hex(4)];[temp:hex(4)]\n");
	fprintf(stderr, "     unix_ts - unix time stamp (unsigned int/long ; UTC)\n");
	fprintf(stderr, "     mode - display mode (1 char) - 'r' for Raw\n");
	fprintf(stderr, "     result - data read result code. 0 for OK, otherwise error (see source code).\n");
	fprintf(stderr, "     humi - hexodecimal (4char len) value of humidity (humidity resolution is 0.1%%)\n");
	fprintf(stderr, "     temp - hexodecimal (4char len) value of temperature (resolution 0.1degC)\n");
	fprintf(stderr, "-val \n");
	fprintf(stderr, "     v:[unix_ts:uint];[mode:char];[result:int];[humi:float];[temp:float]\n");
	fprintf(stderr, "     uni_ts, result - see format raw\n");
	fprintf(stderr, "     mode - display mode (1 char) - 'v' for Value\n");
	fprintf(stderr, "     humi - humudity (Rh%%), format: 00.0\n");
	fprintf(stderr, "     temp - temperature (degC), format: [-]00.0\n");
	fprintf(stderr, "-raw -val\n");
	fprintf(stderr, "     v:[unix_ts:uint];[mode];[result:int];[humi:hex(4)];[temp:hex(4)];[humi:float];[temp:float]\n");
	fprintf(stderr, "     Combined raw & val results into one line\n");
	fprintf(stderr, "-info\n");
	fprintf(stderr, "     i:[result:int];[model:hex(4)];[version:hex(2)];[dev_id:hex(8)];[status:hex(2)]\n");
	fprintf(stderr, "     result - data read reslt code. 0 for OK, otherwise error (see source code).\n");
	fprintf(stderr, "     model - hex (4 char) - model code\n");
	fprintf(stderr, "     version - hex (2 char) - sensor version\n");
	fprintf(stderr, "     dev_id - hex (8 char) - sensor unique(?) ID\n");
	fprintf(stderr, "     status - hex (2 char) - sensor status (?)\n");
	fprintf(stderr, "\n");
}

int check_bus(void) {
	if (access(i2c_bus, F_OK) != -1) {
		if (access(i2c_bus, W_OK) != W_OK) {
			return 0;
		} else {
			return 2; // no write access
		}
	} else {
		return 1; // no bus
	}
}

void process_argv(int argc, char* argv[]) {
	
	#ifdef DEBUG
	if (argc > 0) {
		fprintf(stderr, "\nArguments count: %i\n",argc);
		for (int i=0;i<argc;i++) {
			fprintf(stderr, "- [%02i] data: %s\n",i,argv[i]);
		}
	}
	#endif
	
	if (argc > 1) {
		for(int i=1;i<(argc);i++) {
			// set bus number
			if (strcmp(argv[i],"-b") == 0) {
				i++;
				if (i >= argc) {
					fprintf(stderr, "Parameter `%s` value error (no value)",argv[i-1]);
					exit(1);
				}
				int busnum = atoi(argv[i]);
				char bs1[100];
				char bs2[10];
				sprintf(bs2, "%d", busnum);
				strcpy(bs1, "/dev/i2c-");
				strcat(bs1, bs2); // glue together base + bus number
				strcpy(i2c_bus,bs1); // update global bus name
				fprintf(stderr, "Set I2C Bus to: %s\n", i2c_bus);
				int r = check_bus();
				if (r == 1) {
					fprintf(stderr, "I2C bus device `%s` does not exists!\n",i2c_bus);
				} else if (r == 2) {
					fprintf(stderr, "You do not have permission to write to I2C bus device `%s`.\n",i2c_bus);
				}
			// show help
			} else if (strcmp(argv[i], "-h") == 0) {
				print_help();
			// set bus device file
			} else if (strcmp(argv[i], "-bf") == 0) {
				i++;
				if (i >= argc) {
					fprintf(stderr, "Parameter `%s` value error (no value)",argv[i-1]);
					exit(1);
				}
				char bs1[120];
				strcpy(bs1, argv[i]);
				strcpy(i2c_bus,bs1); // update global bus name
				fprintf(stderr, "Set I2C Bus to: %s\n", i2c_bus);
				int r = check_bus();
				if (r == 1) {
					fprintf(stderr, "I2C bus device `%s` does not exists!\n",i2c_bus);
				} else if (r == 2) {
					fprintf(stderr, "You do not have permission to write to I2C bus device `%s`.\n",i2c_bus);
				}
			} else if (strcmp(argv[i],"-raw") == 0) {
				raw_mode=1;
				fprintf(stderr, "AM2321 readings in will be in RAW.\n");
			} else if (strcmp(argv[i],"-val") == 0) {
				val_mode=1;
				fprintf(stderr, "AM2321 readings in will be in human format.\n");
			} else if (strcmp(argv[i],"-info") == 0) {
				info_mode=1;
				fprintf(stderr, "AM2321 read info.\n");
			}
		}
	}
}

int main(int argc, char* argv[]) {
	int r;
	// set up
	strcpy(i2c_bus,"/dev/i2c-1");
	
	// print header
	fprintf(stderr, "AM2321 reader application by saper_2 v.0.1\n");
	process_argv(argc, argv);
	
	fprintf(stderr, "Using i2c bus device: %s\n",i2c_bus);
	
	if (info_mode) {
		uint8_t data[16];
		memset(data,0xff,16);
		r = am2321_read_info(data, i2c_bus);
		fprintf(stderr,"Info read result %i\n",r);
		fprintf(stderr,"Info data: ");
		for(int i=0;i<8;i++) fprintf(stderr, "%02X ",data[i]);
		fprintf(stderr,"\n");
		
		uint16_t model = data[0]<<8 | data[1];
		uint8_t version = data[2];
		uint32_t devid = data[3]<<24 | data[4]<<16 | data[5]<<8 | data[6];
		uint8_t stat = data[7];
		fprintf(stderr, "Model: 0x%04X (%iu)\n",model,model);
		fprintf(stderr, "Version: 0x%02X (%iu)\n",version,version);
		fprintf(stderr, "Device ID: 0x%08X (%iu)\n",devid,devid);
		fprintf(stderr, "Status: 0x%02X (%iu)\n",stat,stat);
		
		// format: i:[res_code];[model];[version];[dev_id];[status]
		printf("i:%i;%04x;%02x;%08x;%02x\n", r, model, version, devid, stat);
		
		if (raw_mode || val_mode) {
			//printf("\n");
			fprintf(stderr,"\nDelay 1sec before reading values...\n");
			delay1us(1*1000*1000);
		}
	}
	
	if (val_mode || raw_mode) {
		am2321_t val;
		time_t tt;
		unsigned long ts = (unsigned)time(NULL);
		time(&tt);
		memset(&val,0,sizeof(val));
		r = am2321_read_raw(&val, i2c_bus);
		fprintf(stderr,"Read values result %i\n",r);
		fprintf(stderr,"Read values data: \n");
		fprintf(stderr, "   humi = 0x%04X (%i , %3.1f %%)\n",val.humi,val.humi, (val.humi*1.0f)/10.0f);
		fprintf(stderr, "   temp = 0x%04X (%i , %3.1f degC)\n",val.temp,val.temp, (val.temp*1.0f)/10.0f);
		struct tm *tti = localtime(&tt);
		fprintf(stderr, "Current time is: %lu - %s\n",ts,asctime(tti));
		
		fprintf(stderr,"\n");
		if (val_mode>0 && raw_mode==0) {
			// val mode
			// format: v:[unix_time_stamp];[mode];[result];[humi];[temp]
			printf("v:%lu;r;%i;%3.1f;%3.1f\n", ts, r, (val.humi*1.0f)/10.0f, (val.temp*1.0f)/10.0f);
		} else if (val_mode==0 && raw_mode>0) {
			// raw mode
			// format: v:[unix_time_stamp];[mode];[result];[hex humi];[hex temp]
			printf("v:%lu;v;%i;%04x;%04x\n", ts, r, val.humi, val.temp);
		} else if (val_mode>0 && raw_mode>0) {
			// raw + val
			// format: v:[unix_time_stamp];[mode];[result];[hex humi];[hex temp];[humi];[temp]
			printf("v:%lu;rv;%i;%04x;%04x;%3.1f;%3.1f\n", ts, r, val.humi, val.temp, (val.humi*1.0f)/10.0f, (val.temp*1.0f)/10.0f);
		} else {
			// no format ?
			// format: x:[unix_time_stamp];[mode];9;FFFF;FFFF;65535;32767
			printf("x:%lu;x;9;FFFF;FFFF;65535;32767\n", ts);
		}
	}
	
	if (val_mode==0 && raw_mode==0 && info_mode==0) {
		print_help();
		fprintf(stderr,"No read mode selected.\n");
		// format: x:[unix_time_stamp=0];[mode=E (=error)];9;FFFF;FFFF;65535;32767
		printf("x:0;E;9;FFFF;FFFF;65535;32767\n");
	}
	
	return 0;
}