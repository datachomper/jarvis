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

#define NUM_PIX 5
#define HELM 0
#define EX 1
#define UNI 2
#define LREP 3
#define RREP 4
Adafruit_NeoPixel pix[NUM_PIX];
#define INTERVAL 50
#define STROBESPEED 250
uint32_t global_color;
int exwife_is_up;

#define NUM_SERVOS 4
// HELM=0 defined by pixels above
#define ROOF 1
#define CRADLE 2
#define FLAP 3

#define HELM_OPEN 600
#define HELM_CLOSE 2200
#define ROOF_OPEN 1600
#define ROOF_CLOSE 1000
#define CRADLE_OPEN 700
#define CRADLE_CLOSE 1500
#define FLAP_OPEN 2300
#define FLAP_CLOSE 800

struct serv {
	Servo servo;
	int pin;
	int cur = 1500;
	int end = 1500;
	int speed = 10;
} servos[4];

int exwife_is_closed() {
	if (servos[ROOF].cur == ROOF_CLOSE)
		return 1;
	else
		return 0;
}

int exwife_is_open() {
	if (servos[ROOF].cur == ROOF_OPEN)
		return 1;
	else
		return 0;
}

void init_pix() {
	pix[EX] = Adafruit_NeoPixel(13, 10, NEO_GRB + NEO_KHZ800);
	pix[HELM] = Adafruit_NeoPixel(16, 8, NEO_GRB + NEO_KHZ800);
	pix[UNI] = Adafruit_NeoPixel(24, 5, NEO_GRB + NEO_KHZ800);
	pix[LREP] = Adafruit_NeoPixel(12, 6, NEO_GRB + NEO_KHZ800);
	pix[RREP] = Adafruit_NeoPixel(12, 7, NEO_GRB + NEO_KHZ800);

	for (int i = 0; i < NUM_PIX; i++) {
		pix[i].begin();
	}
}

void color_wipe(uint32_t c) {
	for (int i = 0; i < NUM_PIX; i++) {
		Adafruit_NeoPixel *p = &pix[i];

		#if 0
		// Skip exwife pixels if it's closed
		if ((i == EX) && exwife_is_closed())
			continue;

		p->clear();
		p->show();

		for (int j = 0; j < p->numPixels(); j++) {
			p->setPixelColor(j, c);
			p->show();
			// 40ms is for 77 pixel changes over 3 seconds
			delay(50);
		}
		#endif

		// Forget the wipe animation for now, it's buggy
		set_pixel_color(i, c);
		set_pixel_color(i, c);
	}
}

void clear_pixels(int a) {
	Adafruit_NeoPixel *p = &pix[a];
	p->clear();
	p->show();
}

void set_pixel_color(int a, uint32_t c) {
	Adafruit_NeoPixel *p = &pix[a];

	for (int j = 0; j < p->numPixels(); j++) {
		p->setPixelColor(j, c);
	}
	p->show();
}

void set_all_pixel_bright(int val) {
	for (int i = 0; i < NUM_PIX; i++) {
		Adafruit_NeoPixel *p = &pix[i];
		p->setBrightness(val);
		p->show();
	}
}

void set_all_pixel_color(uint32_t c) {
	for (int i = 0; i < NUM_PIX; i++) {
		Adafruit_NeoPixel *p = &pix[i];

		// Skip exwife pixels if it's closed
		if ((i == EX) && exwife_is_closed())
			continue;

		for (int j = 0; j < p->numPixels(); j++) {
			p->setPixelColor(j, c);
		}
		p->show();
	}
}

void setup()
{
	servos[HELM].pin = 9;
	servos[ROOF].pin = 11;
	servos[CRADLE].pin = 12;
	servos[FLAP].pin = 13;
	servos[FLAP].speed = 50;

	/* Manually setup the servos to safe starting positions */
	/* Attach all servos */
	for (int i = 0; i < NUM_SERVOS; i++) {
		servos[i].servo.attach(servos[i].pin);
	}

	servos[HELM].servo.writeMicroseconds(HELM_OPEN);
	servos[ROOF].servo.writeMicroseconds(ROOF_OPEN);
	delay(500);
	servos[CRADLE].servo.writeMicroseconds(CRADLE_CLOSE);
	delay(500);
	servos[FLAP].servo.writeMicroseconds(FLAP_CLOSE);
	servos[ROOF].servo.writeMicroseconds(ROOF_CLOSE);
	exwife_is_up = 0;
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

	/* Set all pixels to default color */
	init_pix();
	global_color = Adafruit_NeoPixel::Color(127, 0, 0);
	set_pixel_color(UNI, global_color);
	set_pixel_color(LREP, global_color);
	set_pixel_color(RREP, global_color);

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
	static int servocounter = 0;
	static int excounter = 0;
	static char litpixel[12] = {1,0,0,0,0,0,1,0,0,0,0,0};

	/* exwife light chase */
	if (exwife_is_up) {
		if (excounter > 100) {
			char tmp[12] = {0};
			Adafruit_NeoPixel *p = &pix[EX];

			excounter = 0;
			for (int i = 0; i < 12; i++) {
				if (litpixel[i])
					p->setPixelColor(i, global_color);		
				else
					p->setPixelColor(i, 0);		
			}
			p->show();

			for (int i = 0; i < 12; i++) {
				if (litpixel[i]) {
					if (i == 11)
						tmp[0] = 1;
					else
						tmp[i+1] = 1;
					tmp[i] = 0;
				}
			}

			for (int i = 0; i < 12; i++) {
				litpixel[i] = tmp[i];
			}
		} else {
			excounter++;
		}
	}
}

void strobe() {
	static unsigned long strobe_timeout = 0;
	static int strobe_state = 0;

	if (millis() > strobe_timeout) {
		if (strobe_state)
			set_all_pixel_color(0);
		else
			set_all_pixel_color(global_color);

		strobe_state = ~strobe_state;
		strobe_timeout = millis() + STROBESPEED;
	}
}

void open_faceplate() {
	clear_pixels(HELM);
	servos[HELM].servo.attach(servos[HELM].pin);
	servos[HELM].servo.writeMicroseconds(HELM_OPEN);
	delay(1000);
	servos[HELM].servo.detach();
}

void close_faceplate() {
	servos[HELM].servo.attach(servos[HELM].pin);
	servos[HELM].servo.writeMicroseconds(HELM_CLOSE);
	delay(1000);
	servos[HELM].servo.detach();
	set_pixel_color(HELM, global_color);
}

void open_exwife() {
	servos[FLAP].servo.attach(servos[FLAP].pin);
	servos[FLAP].servo.writeMicroseconds(FLAP_OPEN);

	servos[ROOF].servo.attach(servos[ROOF].pin);
	servos[ROOF].servo.writeMicroseconds(ROOF_OPEN);

	servos[CRADLE].servo.attach(servos[CRADLE].pin);
	servos[CRADLE].servo.writeMicroseconds(CRADLE_OPEN);

	delay(500);
	servos[FLAP].servo.detach();
	servos[ROOF].servo.detach();
	servos[CRADLE].servo.detach();
	exwife_is_up = 1;
}

void close_exwife() {
	exwife_is_up = 0;
	servos[CRADLE].servo.attach(servos[CRADLE].pin);
	servos[CRADLE].servo.writeMicroseconds(CRADLE_CLOSE);

	servos[ROOF].servo.attach(servos[ROOF].pin);
	servos[ROOF].servo.writeMicroseconds(ROOF_CLOSE);

	servos[FLAP].servo.attach(servos[FLAP].pin);
	servos[FLAP].servo.writeMicroseconds(FLAP_CLOSE);

	delay(500);
	servos[CRADLE].servo.detach();
	servos[ROOF].servo.detach();
	servos[FLAP].servo.detach();
	clear_pixels(EX);
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
			open_faceplate();
		} else if (cmd == CLOSE_FACEPLATE) {
			close_faceplate();
		} else if (cmd == LED_COLOR) {
			int r = Serial.parseInt();
			int g = Serial.parseInt();
			int b = Serial.parseInt();

			global_color = Adafruit_NeoPixel::Color(r, g, b);
			color_wipe(global_color);
			partymode = 0;
		} else if (cmd == LED_BRIGHTNESS) {
			int value = Serial.parseInt();
			set_all_pixel_bright(value);
		} else if (cmd == PARTY_MODE) {
			if (Serial.parseInt())
				partymode = 1;
			else
				partymode = 0;

			// Reset to previous color
			if (!partymode) {
				color_wipe(global_color);
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
	
			if (state == 1) {
				open_exwife();
			} else if (state == 0){
				close_exwife();
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
