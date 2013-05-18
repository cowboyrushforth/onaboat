// onaboat v 0.0.1
// autonomic floating bar software

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sched.h>
#include <bcm2835.h>

// pins
#define SENSOR_ONE_TRIGGER 	RPI_V2_GPIO_P1_16 	// GPIO 23
#define SENSOR_ONE_ECHO 	RPI_V2_GPIO_P1_18 	// GPIO 24
#define AFLEX_I2C_SDA  		RPI_V2_GPIO_P1_03	// GPIO 2
#define AFLEX_I2C_SCL  		RPI_V2_GPIO_P1_05	// GPIO 3

// aflex definitions
#define AFLEX_FIRMWARE_VER      0x24
#define AFLEX_READ_REMOTE_CTRL  0x35
#define AFLEX_PIN_MODE          0x40
#define AFLEX_DIGITAL_WRITE     0x41
#define AFLEX_DIGITAL_READ      0x42
#define AFLEX_ANALOG_READ       0x43
#define AFLEX_FREQ_READ         0x44
#define AFLEX_MOTOR_FORWARD     0x50
#define AFLEX_MOTOR_REVERSE     0x51
#define AFLEX_MOTOR_STOP        0x52
#define AFLEX_MOTOR_CURRENT     0x53
#define AFLEX_SWITCH_READ       0x54
#define AFLEX_PRT_STRING        0x10
#define ALFEX_ANALOG_MODE       0x05
#define AFLEX_FREQ_MODE         0x06
#define AFLEX_COMMAND           0xFE // command start byte
#define AFLEX_CMDDELAY          100  // 100uS

double sensor_distance_cm(uint8_t trigger_pin, uint8_t echo_pin) 
{

	bcm2835_gpio_write(trigger_pin, HIGH);
	bcm2835_delayMicroseconds(10);
	bcm2835_gpio_write(trigger_pin, LOW);

	uint8_t level = 0;
	struct timeval start_time, stop_time;
	gettimeofday(&start_time, NULL);
	while(level == 0) {
		level = bcm2835_gpio_lev(echo_pin);
		gettimeofday(&start_time, NULL);
	}
	while(level == 1) {
		level = bcm2835_gpio_lev(echo_pin);
		gettimeofday(&stop_time, NULL);
	}

	int micros;
	if (stop_time.tv_sec > start_time.tv_sec) micros = 1000000L; else micros = 0;
	micros = micros + (stop_time.tv_usec - start_time.tv_usec);

	double distance = (((float)micros)/1000.0) * 34.0;
	distance = distance / 2.0;
	return distance;
}


void full_stop()
{
	// send stop command to motor one
	char buff[3];
	buff[0] = AFLEX_COMMAND;
	buff[1] = AFLEX_MOTOR_STOP;
	buff[2] = 1;
	bcm2835_i2c_write(buff, sizeof(buff));

	// send stop command to motor two
	buff[2] = 2;
	bcm2835_i2c_write(buff, sizeof(buff));
}

void forward(double distance, int motor)
{

	// calculate the pwm level based on distance
	int amount;
	if(distance >= 140.0) {
		amount = 250;
	} else if (distance >= 120.0) {
		amount = 220;
	} else if (distance >= 100.0) {
		amount = 200;
	} else if (distance >= 80.0) {
		amount = 100;
	} else if (distance >= 60.0) {
		amount = 80;
	} else if (distance >= 40.0) {
		amount = 40;
	} else {
		amount = 20;
	}

	// send forward command to desired motor
	char buff[4];
	buff[0] = AFLEX_COMMAND;
	buff[1] = AFLEX_MOTOR_FORWARD;
	buff[2] = motor;
	buff[3] = amount;
	bcm2835_i2c_write(buff, sizeof(buff));
}

void reverse(double distance, int motor)
{
	// calculate the pwm level based on distance
	int amount;
	if(distance >= 15.0) {
		amount = 20;
	} else if (distance >= 12.0) {
		amount = 50;
	} else if (distance >= 9.0) {
		amount = 100;
	} else if (distance >= 6.0) {
		amount = 200;
	} else if (distance >= 3.0) {
		amount = 220;
	} else {
		amount = 250;
	}
	// send reverse command to desired motor
	char buff[4];
	buff[0] = AFLEX_COMMAND;
	buff[1] = AFLEX_MOTOR_REVERSE;
	buff[2] = motor;
	buff[3] = amount;
	bcm2835_i2c_write(buff, sizeof(buff));

}

int main(int argc, char **argv)
{
	// initialize board
	if (!bcm2835_init()) {
		return 1;
	}

	// setup scheduling fifo and prevent paging
	struct sched_param sp;
	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sp);
	mlockall(MCL_CURRENT | MCL_FUTURE);


	// setup distance sensor
	bcm2835_gpio_fsel(SENSOR_ONE_ECHO, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_pud(SENSOR_ONE_ECHO);
	bcm2835_gpio_fsel(SENSOR_ONE_TRIGGER, BCM2835_GPIO_FSEL_OUTP);


	// initialize i2c
	bcm2835_i2c_begin();
	bcm2835_i2c_setSlaveAddress(0x4A);
	//bcm2835_i2c_setClockDivider()

	while (1)
	{

		double distance = sensor_distance_cm(SENSOR_ONE_TRIGGER, SENSOR_ONE_ECHO);
		printf("Distance: %.2f cm\n", distance);
		bcm2835_delay(60);

		// if we are around 20 cm, do nothing, shut down
		if(distance < 22.0 && distance > 18.0) {
			full_stop();

		// we are less than 18 go backwards
		} else if(distance <= 18.0) {
			reverse(distance, 1);

		// if we are farther away, go forward
		} else if(distance >= 22.0) {
			forward(distance, 1);			
		}


	}

	// todo - put this in sighandler
	bcm2835_i2c_end();

	return 0;
}
