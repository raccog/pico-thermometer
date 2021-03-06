/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

// DHT11 reading result
typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

// Pins
const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;

const uint D1_PIN = 16;
const uint D2_PIN = 17;
const uint D3_PIN = 18;
const uint D4_PIN = 19;

const uint A_PIN = 2;
const uint B_PIN = 3;
const uint C_PIN = 4;
const uint D_PIN = 5;
const uint E_PIN = 6;
const uint F_PIN = 7;
const uint G_PIN = 8;
const uint P_PIN = 9;

const uint BUTTON_PIN = 26;

const uint DIGIT_PINS[] = {
	D1_PIN, D2_PIN, D3_PIN, D4_PIN, 
};

const uint SEGMENT_PINS[] = {
	A_PIN, B_PIN, C_PIN, D_PIN,
	E_PIN, F_PIN, G_PIN, P_PIN
};

// 7-segment bitmasks
const bool DIGIT_0[] = {1, 1, 1, 1, 1, 1, 0, 0};
const bool DIGIT_1[] = {0, 1, 1, 0, 0, 0, 0, 0};
const bool DIGIT_2[] = {1, 1, 0, 1, 1, 0, 1, 0};
const bool DIGIT_3[] = {1, 1, 1, 1, 0, 0, 1, 0};
const bool DIGIT_4[] = {0, 1, 1, 0, 0, 1, 1, 0};
const bool DIGIT_5[] = {1, 0, 1, 1, 0, 1, 1, 0};
const bool DIGIT_6[] = {1, 0, 1, 1, 1, 1, 1, 0};
const bool DIGIT_7[] = {1, 1, 1, 0, 0, 0, 0, 0};
const bool DIGIT_8[] = {1, 1, 1, 1, 1, 1, 1, 0};
const bool DIGIT_9[] = {1, 1, 1, 0, 0, 1, 1, 0};

const bool *DIGIT_MASKS[] = {
	DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4,
	DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9
};

// Button state
uint64_t last_press = 0;
bool should_read = false;

// 7-segment display state
bool display[][8] = {
	{0},
	{0},
	{0},
	{0},
};

void read_from_dht(dht_reading *result);

/*
 *	Sets 7-segment digit state
 */
void set_digit(uint selector, uint value) {
	memcpy(&display[selector], DIGIT_MASKS[value], sizeof(bool[8]));
}

/*
 *	Turns off all 7-segment display pins
 */
void display_off() {
	// turn off digit pins
	for (uint i = 0; i < 4; ++i) {
		gpio_put(DIGIT_PINS[i], 1);
	}

	// turn off segment pins
	for (uint i = 0; i < 8; ++i) {
		gpio_put(SEGMENT_PINS[i], 0);
	}
}

/*
 *	Display a single digit of the 7-segment display and sleep for 2 milliseconds
 */
void display_digit(uint selector) {
	// set digit pins
	for (uint i = 0; i < 4; ++i) {
		gpio_put(DIGIT_PINS[i], (selector == i) ? 0 : 1);
	}

	// set segment pins
	for (uint i = 0; i < 8; ++i) {
		gpio_put(SEGMENT_PINS[i], display[selector][i]);
	}

	sleep_ms(2);
}

/*
 *	Display all digits of 7-segment display for 8 seconds
 */
void display_all() {
	for (uint i = 0; i < 1000; ++i) {
		for (uint j = 0; j < 4; ++j) {
			display_digit(j);
		}
	}
	display_off();
}

void button_callback() {
	// disable interrupts
	gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, false);

	// read from dht
	should_read = true;

	// enable interrupts
	gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, button_callback);
}

void print_dht_reading() {
	// read from dht
	dht_reading reading;
	read_from_dht(&reading);

	// print reading to 7-segment pins
	float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
	set_digit(0, (uint)(fahrenheit) / 10);
	set_digit(1, (uint)(fahrenheit) % 10);
	set_digit(2, (uint)(reading.humidity) / 10);
	set_digit(3, (uint)(reading.humidity) % 10);
	display_all();
}

int main() {
	// declare binary info
	bi_decl(bi_program_name("7-segment Thermometer"));
	bi_decl(bi_program_description("A thermometer using a DHT11 and a 7-segment display"));
	bi_decl(bi_pin_mask_with_name(0x1f << (D1_PIN), "7-segment digit pins 0-3"));
	bi_decl(bi_pin_mask_with_name(0xff << (A_PIN), "7-segment segment pins 0-7"));
	bi_decl(bi_1pin_with_name(DHT_PIN, "DHT11 pin"));
	bi_decl(bi_1pin_with_name(BUTTON_PIN, "Button input"));
	bi_decl(bi_program_version_string("0.1.0"));
	bi_decl(bi_program_url("https://github.com/raccog/pico-thermometer"));

	// init gpio pins
    gpio_init(DHT_PIN);
    gpio_init(BUTTON_PIN);
	for (uint i = 0; i < sizeof(DIGIT_PINS) / sizeof(uint); ++i) {
		gpio_init(DIGIT_PINS[i]);
	}
	for (uint i = 0; i < sizeof(SEGMENT_PINS) / sizeof(uint); ++i) {
		gpio_init(SEGMENT_PINS[i]);
	}

	// set gpio pin directions
	gpio_set_dir(BUTTON_PIN, GPIO_IN);
	for (uint i = 0; i < sizeof(DIGIT_PINS) / sizeof(uint); ++i) {
		gpio_set_dir(DIGIT_PINS[i], GPIO_OUT);
	}
	for (uint i = 0; i < sizeof(SEGMENT_PINS) / sizeof(uint); ++i) {
		gpio_set_dir(SEGMENT_PINS[i], GPIO_OUT);
	}

	// pull down button pin
	gpio_pull_down(BUTTON_PIN);

	// set button pin interrupt
	gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, button_callback);

	// main loop
    while (1) {
		if (should_read) {
			should_read = false;

			// ensure it has been 2 seconds since the last button press
			uint64_t current_time = time_us_64();
			if (current_time <= last_press + 2000000) {
				continue;
			} else {
				last_press = current_time;
			}

			// print reading
			print_dht_reading();
		}
		sleep_ms(10);
    }
}

/*
 * Taken from https://github.com/raspberrypi/pico-examples/blob/master/gpio/dht_sensor/dht.c
 *
 */
void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);
    gpio_set_dir(DHT_PIN, GPIO_IN);
	sleep_us(40);

    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint count = 0;
        while (gpio_get(DHT_PIN) == last) {
            count++;
            busy_wait_us_32(1);
            if (count == 255) break;
        }
        last = gpio_get(DHT_PIN);
        if (count == 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (count > 50) data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) {
            result->temp_celsius = -result->temp_celsius;
        }
    }
}
