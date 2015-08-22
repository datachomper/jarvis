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

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(13, 10, NEO_GRB + NEO_KHZ800);
#define INTERVAL 50
#define STROBESPEED 250

Servo helm;
Servo roof;
Servo cradle;
Servo flap;

void setup()
{
	Serial.begin(9600);
	pixels.begin();

	for (int i = 0; i < pixels.numPixels(); i++) {
		pixels.setPixelColor(i, pixels.Color(1, 5, 5));
	}
	pixels.show();

	/* This seems to prevent strange crosstalk to servos that aren't
 	 * initialized yet */
	helm.attach(9);
	helm.detach();

	roof.attach(11);
	roof.detach();

	cradle.attach(12);
	cradle.detach();

	flap.attach(13);
	flap.detach();
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

			helm.attach(9);
			//TODO: should this be .writeMicroseconds()?
			helm.write(600);
			delay(1000);
			helm.detach();
		} else if (cmd == CLOSE_FACEPLATE) {
			helm.attach(9);
			//TODO: should this be .writeMicroseconds()?
			helm.write(2100);
			delay(1000);
			helm.detach();
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
				helm.attach(9);
				helm.write(pwm);
				Serial.print("faceplate moved to ");
				Serial.println(pwm);
				delay(1000);
				helm.detach();
		} else if (cmd == ROOF_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				roof.attach(11);
				roof.writeMicroseconds(pwm);
				Serial.print("roof moved to ");
				Serial.println(pwm);
				delay(1000);
				roof.detach();
		} else if (cmd == CRADLE_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				cradle.attach(12);
				cradle.writeMicroseconds(pwm);
				Serial.print("cradle moved to ");
				Serial.println(pwm);
				delay(1000);
				cradle.detach();
		} else if (cmd == FLAP_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				flap.attach(13);
				flap.writeMicroseconds(pwm);
				Serial.print("flap moved to ");
				Serial.println(pwm);
				delay(1000);
				flap.detach();
		}
	}

	if (partymode) {
		if (millis() > timeout) {
			timeout = millis() + INTERVAL;
			strobe();
		}
	}
}
