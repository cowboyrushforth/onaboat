#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sched.h>
#include <bcm2835.h>
#include "pwm.c"

#define MOTOR_ONE_FORWARD RPI_V2_GPIO_P1_24
#define MOTOR_ONE_REVERSE RPI_V2_GPIO_P1_26
#define SENSOR_ONE_TRIGGER RPI_V2_GPIO_P1_16
#define SENSOR_ONE_ECHO RPI_V2_GPIO_P1_18

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
        clear_channel_gpio(0, MOTOR_ONE_REVERSE);
        clear_channel_gpio(0, MOTOR_ONE_FORWARD);
//	bcm2835_gpio_write(MOTOR_ONE_FORWARD, LOW);
//	bcm2835_gpio_write(MOTOR_ONE_REVERSE, LOW);
}

void forward(double distance)
{

	int amount;
	if(distance >= 140.0) {
		amount = 600;
	} else if (distance >= 120.0) {
		amount = 500;
	} else if (distance >= 100.0) {
		amount = 400;
	} else if (distance >= 80.0) {
		amount = 300;
	} else if (distance >= 60.0) {
		amount = 200;
	} else if (distance >= 40.0) {
		amount = 100;
	} else {
		amount = 50;
	}
        add_channel_pulse(0, MOTOR_ONE_FORWARD,0,amount);
        clear_channel_gpio(0, MOTOR_ONE_REVERSE);
	//bcm2835_gpio_write(MOTOR_ONE_FORWARD, HIGH);
	//bcm2835_gpio_write(MOTOR_ONE_REVERSE, LOW);
}

void reverse(double distance)
{
	int amount;
	if(distance >= 15.0) {
		amount = 50;
	} else if (distance >= 12.0) {
		amount = 150;
	} else if (distance >= 9.0) {
		amount = 300;
	} else if (distance >= 6.0) {
		amount = 400;
	} else if (distance >= 3.0) {
		amount = 500;
	} else {
		amount = 600;
	}
        add_channel_pulse(0, MOTOR_ONE_REVERSE,1,amount);
        clear_channel_gpio(0, MOTOR_ONE_FORWARD);
	//bcm2835_gpio_write(MOTOR_ONE_FORWARD, LOW);
	//bcm2835_gpio_write(MOTOR_ONE_REVERSE, HIGH);
}

int main(int argc, char **argv)
{
	// Use for testing
	//  bcm2835_set_debug(1);

	if (!bcm2835_init())
		return 1;


	// setup scheduling hack
	struct sched_param sp;
	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sp);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	// setup pins
	bcm2835_gpio_fsel(MOTOR_ONE_FORWARD, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(MOTOR_ONE_REVERSE, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_fsel(SENSOR_ONE_ECHO, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_pud(SENSOR_ONE_ECHO);
	bcm2835_gpio_fsel(SENSOR_ONE_TRIGGER, BCM2835_GPIO_FSEL_OUTP);

	// Setup channel
	set_loglevel(99);
        setup(10, DELAY_VIA_PWM);
	int channel = 0;
	int subcycle_time_us = 10000; //10ms;
	init_channel(channel, subcycle_time_us);
	init_gpio(MOTOR_ONE_FORWARD);
	init_gpio(MOTOR_ONE_REVERSE);


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
			reverse(distance);

			// if we are farther away, go forward
		} else if(distance >= 22.0) {
			forward(distance);			
		}


	}

        // All done
        shutdown();

	return 0;
}
