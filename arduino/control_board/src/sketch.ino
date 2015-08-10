#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800);

void setup()
{
	Serial.begin(9600);
	pixels.begin();

	for (int i = 0; i < 12; i++) {
		pixels.setPixelColor(i, pixels.Color(0, 150, 0));
	}
	pixels.show();
}

void loop()
{
	if (Serial.available() > 0) {
		char cmd = Serial.read();
		if (cmd == '1') {
			for (int i = 0; i < 12; i++) {
				pixels.setPixelColor(i, pixels.Color(0, 150, 0));
			}
			pixels.show();
		} else if (cmd == '0') {
			for (int i = 0; i < 12; i++) {
				pixels.setPixelColor(i, pixels.Color(0, 0, 0));
			}
			pixels.show();
		} else {
			/* invalid command */
		}
	}
}
