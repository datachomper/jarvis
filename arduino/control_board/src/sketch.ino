#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#define OPEN_FACEPLATE 1
#define CLOSE_FACEPLATE 2
#define LED_COLOR 3
#define LED_BRIGHTNESS 4
#define PARTY_MODE 5
#define FACEPLATE_SERVO 6
#define ROOF_SERVO 7
#define CRADLE_SERVO 8
#define FLAP_SERVO 9
#define EXWIFE_STATE 10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(13, 10, NEO_GRB + NEO_KHZ800);
#define INTERVAL 50
#define STROBESPEED 250

#define NUM_SERVOS 4
#define HELM 0
#define ROOF 1
#define CRADLE 2
#define FLAP 3

#define ROOF_OPEN 1600
#define ROOF_CLOSE 1000
#define CRADLE_OPEN 700
#define CRADLE_CLOSE 1500
#define FLAP_OPEN 2000
#define FLAP_CLOSE 1500

struct serv {
	Servo servo;
	int pin;
	int cur = 1500;
	int end = 1500;
	int speed = 10;
} servos[4];

void setup()
{
	servos[HELM].pin = 9;
	servos[ROOF].pin = 11;
	servos[CRADLE].pin = 12;
	servos[FLAP].pin = 13;

	/* Manually setup the servos to safe starting positions */
	/* Attach all servos */
	for (int i = 0; i < NUM_SERVOS; i++) {
		servos[i].servo.attach(servos[i].pin);
	}

	servos[HELM].servo.writeMicroseconds(1500);
	servos[ROOF].servo.writeMicroseconds(ROOF_OPEN);
	delay(500);
	servos[CRADLE].servo.writeMicroseconds(CRADLE_CLOSE);
	delay(500);
	servos[FLAP].servo.writeMicroseconds(FLAP_CLOSE);
	servos[ROOF].servo.writeMicroseconds(ROOF_CLOSE);
	delay(500);

	/* Record final servo positions */
	servos[ROOF].cur = servos[ROOF].end = ROOF_CLOSE;
	servos[CRADLE].cur = servos[CRADLE].end = CRADLE_CLOSE;
	servos[FLAP].cur = servos[FLAP].end = FLAP_CLOSE;

	/* Detach all servos */
	for (int i = 0; i < NUM_SERVOS; i++) {
		servos[i].servo.detach();
	}

	Serial.begin(9600);
	pixels.begin();

	for (int i = 0; i < pixels.numPixels(); i++) {
		pixels.setPixelColor(i, pixels.Color(1, 5, 5));
	}
	pixels.show();

	cli();
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2  = 0;
	// set compare match register for 1khz increments
	// 16MHz(clock)/64(prescaler)/249(compare reg)) = 1Khz
	OCR2A = 249;

	// turn on CTC mode
	TCCR2A |= (1 << WGM21);
	// Set CS21 bit for 64 prescaler
	TCCR2B |= (1 << CS22);   
	// enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2A);

	sei();
}

/* Currently being called at 1Khz */
ISR(TIMER2_COMPA_vect) {
	static int counter = 0;

	/* Update @ 1Khz/10 = 100Hz */
	if (counter > 10) {
		counter = 0;

		for (int i = 0; i < 4; i++) {
			struct serv *s = &servos[i];

			if (s->cur != s->end) {
				if (!s->servo.attached())
					s->servo.attach(s->pin);

				if (s->end > s->cur) {
					s->cur += s->speed;
					if (s->cur > s->end)
						s->cur = s->end;
				} else {
					s->cur -= s->speed;
					if (s->cur < s->end)
						s->cur = s->end;
				}

				s->servo.writeMicroseconds(s->cur);
			} else if (s->servo.attached()) {
				/* Detach servo once it reaches final position */
				s->servo.detach();
			}
		}
	} else {
		counter++;
	}
}

void strobe() {
	static unsigned long strobe_timeout = 0;
	static int strobe_state = 0;

	if (millis() > strobe_timeout) {
		for (int i = 0; i < pixels.numPixels(); i++) {
			if (strobe_state)
				pixels.setPixelColor(i, pixels.Color(0, 0, 0));
			else
				pixels.setPixelColor(i, pixels.Color(255, 255, 255));
		}	
		pixels.show();
		strobe_state = ~strobe_state;
		strobe_timeout = millis() + STROBESPEED;
	}
}

void loop()
{
	static unsigned long timeout = 0;
	static int partymode = 0;

	while (Serial.available() > 0) {
		int cmd = Serial.parseInt();
		Serial.print("cmd: ");
		Serial.println(cmd);

		if (cmd == OPEN_FACEPLATE) {
			for (int i = 0; i < pixels.numPixels(); i++) {
				pixels.setPixelColor(i, pixels.Color(0, 0, 0));
			}
			pixels.show();

			servos[HELM].end = 600;
		} else if (cmd == CLOSE_FACEPLATE) {
			servos[HELM].end = 1000;
			for (int i = 0; i < pixels.numPixels(); i++) {
				pixels.setPixelColor(i, pixels.Color(126, 0, 0));
			}
			pixels.show();
		} else if (cmd == LED_COLOR) {
			int red = Serial.parseInt();
			int green = Serial.parseInt();
			int blue = Serial.parseInt();

			Serial.print(red, HEX);
			Serial.print(green, HEX);
			Serial.println(blue, HEX);
	
			for (int i = 0; i < pixels.numPixels(); i++) {
				pixels.setPixelColor(i, pixels.Color(red, green, blue));
			}

			pixels.show();
			partymode = 0;
		} else if (cmd == LED_BRIGHTNESS) {
			int value = Serial.parseInt();
			pixels.setBrightness(value);
			pixels.show();
		} else if (cmd == PARTY_MODE) {
			if (Serial.parseInt())
				partymode = 1;
			else
				partymode = 0;

			// Reset to 50% red color
			if (!partymode) {
				for (int i = 0; i < pixels.numPixels(); i++) {
					pixels.setPixelColor(i, pixels.Color(126, 0, 0));
				}
				pixels.show();
			}
		} else if (cmd == FACEPLATE_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				servos[HELM].end = pwm;
		} else if (cmd == ROOF_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				servos[ROOF].end = pwm;
		} else if (cmd == CRADLE_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				servos[CRADLE].end = pwm;
		} else if (cmd == FLAP_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				servos[FLAP].end = pwm;
		} else if (cmd == EXWIFE_STATE) {
			int state = Serial.parseInt();
	
			if (state == 0) {
				Serial.println("closing exwife");
				servos[FLAP].end = FLAP_OPEN;
				delay(500);
				servos[ROOF].end = ROOF_OPEN;
				delay(500);
				servos[CRADLE].end = CRADLE_OPEN;
			} else if (state == 1){
				Serial.println("opening exwife");
				servos[CRADLE].end = CRADLE_CLOSE;
				delay(500);
				servos[ROOF].end = ROOF_CLOSE;
				delay(500);
				servos[FLAP].end = FLAP_CLOSE;
			}
		}
	}

	if (partymode) {
		if (millis() > timeout) {
			timeout = millis() + INTERVAL;
			strobe();
		}
	}
}
