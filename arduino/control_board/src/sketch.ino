#include <Adafruit_NeoPixel.h>

#define OPEN_FACEPLATE 1
#define CLOSE_FACEPLATE 2
#define LED_COLOR 3

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800);

void setup()
{
	Serial.begin(9600);
	Serial.println("Control interface online");
	pixels.begin();

	for (int i = 0; i < 12; i++) {
		pixels.setPixelColor(i, pixels.Color(1, 5, 5));
	}
	pixels.show();
}

void loop()
{
	while (Serial.available() > 0) {
		int cmd = Serial.parseInt();

		if (cmd == OPEN_FACEPLATE) {
		} else if (cmd == CLOSE_FACEPLATE) {
		} else if (cmd == LED_COLOR){
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
		}
	}
}
