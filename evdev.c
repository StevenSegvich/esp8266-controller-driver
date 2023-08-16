#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <linux/input.h>
#include <termios.h>
#include <stdbool.h>
#include "evdev.h"
#include "esp8266drv.c"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

#define AILERON 0
#define ELEVATOR 1
#define RUDDER 2
#define MOTOR 3

struct fallback fallback;

int is_event_device(const struct dirent *dir) {
	return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

char* scan_devices(void) {
	struct dirent **namelist;
	int i, ndev, devnum;
	char *filename;

	ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, alphasort);
	if (ndev <= 0) {
		return NULL;
	}

	printf("Available devices:\n");

	for (i = 0; i < ndev; i++) {
		char fname[64];
		int fd = -1;
		char name[256] = "???";

		snprintf(fname, sizeof(fname), "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
		fd = open(fname, O_RDONLY);
		if (fd >= 0) {
			ioctl(fd, EVIOCGNAME(sizeof(name)), name);
			close(fd);
		}
		printf("%s:  %s\n", fname, name);
		free(namelist[i]);
	}

	fprintf(stderr, "Select the device event number [0-%d]: ", ndev - 1);
	scanf("%d", &devnum);

	if (devnum >= ndev || devnum < 0) {
		return NULL;
	}

	asprintf(&filename, "%s/%s%d", DEV_INPUT_EVENT, EVENT_DEV_NAME, devnum);
	return filename;
}

static int print_device_info(int fd) {
	int i, j;
	int version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];

	if (ioctl(fd, EVIOCGVERSION, &version)) {
		perror("can't get version");
		return 1;
	}
	printf("Input driver version is %d.%d.%d\n", 
	       version >> 16, (version >> 8) & 0xff, version & 0xff);

	ioctl(fd, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
		id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	printf("Supported events:\n");
	for (i = 0; i < EV_MAX; i++) {
 		if (test_bit(i, bit[0])) {
			printf("  Event type %d\n", i);
			if (!i) continue;
			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
			for (j = 0; j < KEY_MAX; j++) {
				if (test_bit(j, bit[i])) {
					printf("%d, ", j);
				}
			}
			printf("\n");
		}
	}
	return 0;
}

void packet_initalizer(struct packet *packet){
	int start = 128;

	packet->aileron = start;
	packet->elevator = start;
	packet->rudder = start;
	packet->motor = start;
}

/* I am removing trim temporarily as it is increasing the complexity of the project and i just need to have something done rather than a bunch of theory. 
In the future, it also needs to be re implimented to function properly with the decimal system for numbers. 


void trim_initalizer(struct trim *trim){
	trim->aileron = 0x00;
	trim->elevator = 0x00;
	trim->rudder = 0x00;
}

unsigned int apply_trim(struct trim *trim, signed int math, int flag){
	if(flag == 0){
		math = math + trim->aileron;
	}else if(flag == 1){
		math = math + trim->elevator;
	}else if(flag == 2){
		math = math + trim->rudder;
	}
	if(math > 99){
		math = 99;
	}else if(math < 0){
		math = 0;
	}
	return math;
}

void set_trim(struct packet *packet, struct trim *trim, int flag, bool increase){
	char trimval = 0x00;
	if(increase == true){
		trimval = 5;
	}else{
		trimval = -5;
	}
	//this can be changed if it feels like too much/little
	if(flag == 0){
		trim->aileron = trim->aileron + trimval;
		packet_update(packet, trim, fallback.aileron, 0);
	}else if(flag == 1){
		trim->elevator = trim->elevator + trimval;
		packet_update(packet, trim, fallback.elevator, 1);
	}else if(flag == 2){
		trim->rudder = trim->rudder + trimval;
		packet_update(packet, trim, fallback.rudder, 2);
	}
	
}

//Brainstorming for trim
//EV.VALUE = 1 MEANS PRESS, EV.VALUE = 0 MEANS RELEASE 
//If button is pressed, write a value to something, updating if the release is registered
//If the bumpers are pressed, the first thing that happens is a check to see if the value is set indicating a button being held down
//If it isnt, the rudder trim is adjusted, if it is, the elevator is adjusted
//AIleron trim is included but is currently unused
//x button on the controller will be for switching the trim

*/

void packet_generator(struct packet *packet, signed int value, int flag){
	unsigned int math;
	math = ((value + 32768) * 255) / 65536;

	packet_update(packet, math, flag);
}

void rudder_generator(struct packet *packet, int value, int flag){
	unsigned int math;
	//printf("%i\n", flag);
	//printf("%i\n", value);
	if(flag == 0){
		math = ((1023-value)*128)/1023;
	}else if (flag == 1){
		math = ((value*128)/1023) + 128;
	}
	//printf("%i\n", math);
	packet_update(packet, math, 2);
}
//This function needs to be reworked at some point. Maybe split into a function to preform math on the value passed, checking if its for the control surfaces vs the motor, as rudders are controled by the triggers they need a different mathematical function preformed
//Also helps in a situation such as the trim set in which i need to call the packet update to apply the trim, rather than waiting for the next update to the corresponding control
void packet_update(struct packet *packet, unsigned int math, int flag){
	//Maybe move this to after the deadzone which is just below
	//math = apply_trim(trim, math, flag);
	if(flag == 0){
		fallback.aileron = math;
		packet->aileron = math;
		//snprintf(buff, 3, "%02u", math % 100);
		//memcpy(packet->aileron, buff, 2);
	}else if(flag == 1){
		fallback.elevator = math;
		packet->rudder = math;
		//snprintf(buff, 3, "%02u", math % 100);
		//memcpy(packet->elevator, buff, 2);
	}else if(flag == 2){
		fallback.rudder = math;
		packet->rudder = math;
		//snprintf(buff, 3, "%02u", math % 100);
		//memcpy(packet->rudder, buff, 2);
	}else if(flag == 3){
		fallback.motor = math;
		packet->motor = math;
		//snprintf(buff, 3, "%02u", math % 100);
		//memcpy(packet->motor, buff, 2);
	}

	//THIS IS FOR TESTING !!! REMOVE !!!
	
	printf("%x", packet->flag);
	printf(" ");
	printf("%d", packet->aileron);
	printf(" ");
	printf("%d", packet->elevator);
	printf(" ");
	printf("%d", packet->rudder);
	printf(" ");
	printf("%d", packet->motor);
	printf(" ");
	printf("%x", packet->endflag);
	printf("\n");

}

int packet_out(struct packet *packet){
	int fd, len;
	struct termios options;
	char packetBuff[100];
	//memcpy(packet->flag, "\xaa", 1);
	packet->endflag = '\n';
	packet->flag = '\xaa';	
	fd = open("/dev/serial/by-id/usb-FTDI_TTL232R-3V3_FTFAZZHZ-if00-port0", O_RDWR | O_NDELAY | O_NOCTTY);
	if(fd < 0) {
		perror("Error opening serial port");
		return -1;
	}


	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;


	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);

	len = write(fd, packet, 10);
	printf("Wrote %d bytes over GPIO\n", len);
	int n = read(fd, packetBuff, sizeof(packetBuff));
	printf("%s", packetBuff);
	
	close(fd);
	return 0;	
}


int print_events(int fd) {
	struct input_event ev;
	unsigned int size;
	struct packet packet;
	//unsigned int math = 0;
	printf("Testing ... (interrupt to exit)\n");
	packet_initalizer(&packet);

	
	if(sendATCommand("AT+RST") != 0){
		printf("unable to talk to esp8266");
		return EXIT_FAILURE;
	}
	sleep(2);
	int err = sendATCommand("ATE0");
	sleep(2);
	err = sendATCommand("AT+CWMODE=1");
	sleep(2);
	err = sendATCommand("AT+CWJAP=\"ESP8266\",\"testplane\"");
	sleep(5);
	

	
	

	/*
	while(1){
		size = read(fd, &ev, sizeof(struct input_event));
		if(ev.code == 5 || ev.code == 2){
			printf("%i\n", ev.value);
		}
	}
	*/
	
	while (1) {
		sleep(.5);
		size = read(fd, &ev, sizeof(struct input_event));

		if (size < sizeof(struct input_event)) {
			printf("expected %u bytes, got %u\n", sizeof(struct input_event), size);
			perror("\nerror reading");
			return EXIT_FAILURE;
		}
//This is for pitch (elevator)
		if(ev.code == 1){
			if(ev.value >= 2048 || ev.value <= -2048){	
				//printf("type: %i, code: %i, value: %i\n", ev.type, ev.code, ev.value);
				//math = ((ev.value + 32768) * 100) / 65536;
				packet_generator(&packet, ev.value, 1);
				packet_out(&packet);
			}else{
				packet_update(&packet, 128, 1);
				packet_out(&packet);
			}
			
		}
//this is for roll control (aileron)	
		else if(ev.code == 3){
			if(ev.value >= 2048 || ev.value <= -2048){
				packet_generator(&packet, ev.value, 0);
				packet_out(&packet);
			}else{
				packet_update(&packet, 128, 0);
				packet_out(&packet);
			}
		}

//This is for yaw (rudder)
		else if(ev.code == 5 || ev.code == 2){
			if(ev.value > 50){
				rudder_generator(&packet, ev.value, ev.code % 2);
				packet_out(&packet);
			}else{
				packet_update(&packet, 128, ev.code % 2);
				packet_out(&packet);
			}
		}
/*
//This is for x button
		else if(ev.code == 307){
			if(ev.value == 1){
				trim_switch = 1;
			}else if(ev.value == 0){
				trim_switch = 0;
			}
		}
//This is for right bumper
		else if(ev.code == 311){
			if(trim_switch == 1){
				set_trim(&packet, ELEVATOR, true);
			}else{
				set_trim(&packet, RUDDER, true);
			}
		}
//This is for left bumper


		else if(ev.code == 310){
			if(trim_switch == 1){
				set_trim(&packet, ELEVATOR, false);
			}else{
				set_trim(&packet, RUDDER, false);
			}
		}
		//else if(ev.code != 0 && ev.code != 1 && ev.code != 3 && ev.code != 4){
		//	printf("type: %i, code: %i, value: %i\n", ev.type, ev.code, ev.value);
		//}
		*/

		//printf("Event: time %ld.%06ld, ", ev.time.tv_sec, ev.time.tv_usec);
		//printf("type: %i, code: %i, value: %i\n", ev.type, ev.code, ev.value);
	}
	
}

int main () {
	int fd, grabbed;
	char *filename;

	if (getuid() != 0) {
		fprintf(stderr, "Not running as root, no devices may be available.\n");
	}

	filename = scan_devices();
	if (!filename) {
		fprintf(stderr, "Device not found\n");
		return EXIT_FAILURE;
	}

	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror("");
		if (errno == EACCES && getuid() != 0) {
			fprintf(stderr, "You do not have access to %s. Try "
					"running as root instead.\n", filename);
		}
		return EXIT_FAILURE;
	}

	free(filename);

	if (print_device_info(fd)) {
		return EXIT_FAILURE;
	}

	grabbed = ioctl(fd, EVIOCGRAB, (void *) 1);
	ioctl(fd, EVIOCGRAB, (void *) 0);
	if (grabbed) {
		printf("This device is grabbed by another process. Try switching VT.\n");
		return EXIT_FAILURE;
	}

	return print_events(fd);
}

