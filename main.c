/*
PIN ASSIGNMENTS:
PA0  IR Sensor 1 input (far left)
PA1  IR Sensor 2 input (left)
PA2  IR Sensor 3 input (center)
PA3  IR Sensor 4 input (right)
PA4  IR Sensor 5 input (far right)
PB0  Status LED output (solid ON when following line)
PB1  Alert LED output (blinks fast when line is lost)
PB2  L298N IN3 output (left motor reverse)
PB3  L298N IN4 output (left motor forward)
PB4  Buzzer output
PD4  L298N ENA output (right motor PWM speed) � OC1B
PD5  L298N ENB output (left motor PWM speed)  � OC1A
PD6  L298N IN1 output (right motor forward)
PD7  L298N IN2 output (right motor reverse)
*/

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#define DIR_LEFT  0
#define DIR_RIGHT 1
uint8_t last_direction = DIR_LEFT;

void init_peripherals() {
	DDRA &= ~0x1F;
	DDRB |=  (1 << PB0);
	DDRB |=  (1 << PB1);
	DDRB |=  (1 << PB2) | (1 << PB3);
	DDRB |=  (1 << PB4);
	DDRD |=  0xF0;

	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);  // Fast pwm
	TCCR1B = (1 << CS11);  // Prescaler by 8
}

void set_motors(uint8_t left_speed, uint8_t right_speed) {
	if (left_speed == 0 && right_speed == 0) {
		PORTD &= ~((1 << PD6) | (1 << PD7));
		PORTB &= ~((1 << PB2) | (1 << PB3));
		OCR1A = 0;
		OCR1B = 0;
		} else {
		PORTD |=  (1 << PD6);
		PORTD &= ~(1 << PD7);
		PORTB |=  (1 << PB3);
		PORTB &= ~(1 << PB2);
		OCR1B = right_speed;
		OCR1A = left_speed;
	}
}

void spin_right() {
	PORTD &= ~(1 << PD6);   // IN1 LOW
	PORTD |=  (1 << PD7);   // IN2 HIGH � right backward
	PORTB |=  (1 << PB3);   // IN4 HIGH � left forward
	PORTB &= ~(1 << PB2);   // IN3 LOW
	OCR1B = 120;
	OCR1A = 120;
}

void spin_left() {
	PORTD |=  (1 << PD6);   // IN1 HIGH � right forward
	PORTD &= ~(1 << PD7);   // IN2 LOW
	PORTB &= ~(1 << PB3);   // IN4 LOW
	PORTB |=  (1 << PB2);   // IN3 HIGH � left backward
	OCR1B = 120;
	OCR1A = 120;
}

uint8_t line_detected() {
	return (~(PINA) & 0x1F);
}

int main(void) {
	init_peripherals();

	// blink status LED 3 times fast on startup
	for (uint8_t i = 0; i < 3; i++) {
		PORTB |=  (1 << PB0);
		_delay_ms(200);
		PORTB &= ~(1 << PB0);
		_delay_ms(200);
	}

	PORTB |= (1 << PB0);        // status LED always ON

	while (1) {
		uint8_t sensors = ~(PINA) & 0x1F;  // inverted � LOW on black

		if (sensors & (1 << PA2)) {
			// center � forward
			PORTB &= ~(1 << PB1);
			PORTB &= ~(1 << PB4);
			set_motors(120, 120);

			} else if (sensors & (1 << PA1)) {
			// left � turn left
			last_direction = DIR_LEFT;
			PORTB &= ~(1 << PB1);
			PORTB &= ~(1 << PB4);
			set_motors(60, 130);

			} else if (sensors & (1 << PA3)) {
			// right � turn right
			last_direction = DIR_RIGHT;
			PORTB &= ~(1 << PB1);
			PORTB &= ~(1 << PB4);
			set_motors(130, 60);

			} else if (sensors & (1 << PA0)) {
			// far left � sharp left
			last_direction = DIR_LEFT;
			PORTB &= ~(1 << PB1);
			PORTB &= ~(1 << PB4);
			set_motors(0, 150);

			} else if (sensors & (1 << PA4)) {
			// far right � sharp right
			last_direction = DIR_RIGHT;
			PORTB &= ~(1 << PB1);
			PORTB &= ~(1 << PB4);
			set_motors(150, 0);

			} else {
			// line lost � search mode
			PORTB |=  (1 << PB4);          // buzzer ON continuously

			while (!line_detected()) {
				// spin toward last known direction
				if (last_direction == DIR_LEFT) {
					spin_left();
					} else {
					spin_right();
				}

				// blink alert LED very fast while searching
				PORTB |=  (1 << PB1);      // alert LED ON
				_delay_ms(50);
				PORTB &= ~(1 << PB1);      // alert LED OFF
				_delay_ms(50);
			}

			// line found again
			set_motors(0, 0);              // brief stop
			PORTB &= ~(1 << PB4);         // buzzer OFF
			PORTB &= ~(1 << PB1);         // alert LED OFF
		}
	}
}