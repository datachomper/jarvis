#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#define OPEN_FACEPLATE 1
#define CLOSE_FACEPLATE 2
#define LED_COLOR 3
#define LED_BRIGHTNESS 4
#define PARTY_MODE 5
#define FACEPLATE_SERVO 6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800);
#define INTERVAL 50
#define STROBESPEED 250

Servo helm_servo;

void setup()
{
	Serial.begin(9600);
	pixels.begin();

	for (int i = 0; i < 12; i++) {
		pixels.setPixelColor(i, pixels.Color(1, 5, 5));
	}
	pixels.show();

}

void strobe() {
	static unsigned long strobe_timeout = 0;
	static int strobe_state = 0;

	if (millis() > strobe_timeout) {
		for (int i = 0; i < 12; i++) {
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
			helm_servo.attach(9);
			helm_servo.write(600);
			delay(1000);
			helm_servo.detach();
		} else if (cmd == CLOSE_FACEPLATE) {
			helm_servo.attach(9);
			helm_servo.write(2100);
			delay(1000);
			helm_servo.detach();
		} else if (cmd == LED_COLOR) {
			int red = Serial.parseInt();
			int green = Serial.parseInt();
			int blue = Serial.parseInt();

			Serial.print(red, HEX);
			Serial.print(green, HEX);
			Serial.println(blue, HEX);
	
			for (int i = 0; i < 12; i++) {
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
				for (int i = 0; i < 12; i++) {
					pixels.setPixelColor(i, pixels.Color(126, 0, 0));
				}
				pixels.show();
			}
		} else if (cmd == FACEPLATE_SERVO) {
			int pwm = Serial.parseInt();
			if ((pwm > 0) && (pwm < 2500))
				helm_servo.write(pwm);
			Serial.print("moved to ");
			Serial.println(pwm);
		}
	}

	if (partymode) {
		if (millis() > timeout) {
			timeout = millis() + INTERVAL;
			strobe();
		}
	}
}
