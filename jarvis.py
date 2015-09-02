#!/usr/bin/python

from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *
import alsaaudio
import time
import RPi.GPIO as GPIO
import random
import wave
import serial
import smbus
import os

bus = smbus.SMBus(1)

button_pressed = False
volume = 30

# For some reason the period size is stuck on this number
PERIOD = 341
FRAME_SIZE = 2
CHUNK = PERIOD * FRAME_SIZE

def scale(value, leftmin, leftmax, rightmin, rightmax):
	leftspan = leftmax - leftmin
	rightspan = rightmax - rightmin
	valuescaled = float(value - leftmin) / float(leftspan)
	return int(rightmin + (valuescaled * rightspan))

def decoder_init():
	hmdir = '/usr/local/share/pocketsphinx/model/en-us/en-us'
	dictd = '/home/pi/jarvis/sphinx_kb/jarvis_v4.dic'
	lmdir = '/home/pi/jarvis/sphinx_kb/jarvis_v4.lm'

	# Create a decoder with certain model
	config = Decoder.default_config()
	config.set_string('-hmm', hmdir)
	config.set_string('-lm', lmdir)
	config.set_string('-dict', dictd)
	config.set_string('-logfn', '/dev/null')
	return Decoder(config)

def decode(decoder, buf):
	# Decode streaming data.
	decoder.start_utt()
	decoder.process_raw(buf, False, True)
	decoder.end_utt()

	# Catch errors pocketsphinx will throw if no audio is detected
	try:
		ret = decoder.hyp().hypstr
	except:
		ret = ''

	return ret

def get_audio():
	buf = ''

	# Setup alsa recording interface for "default" sound card
	rx = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NONBLOCK, 'sysdefault')
	rx.setchannels(1)
	rx.setrate(16000)
	rx.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	rx.setperiodsize(341)

	print "jarvis standing by ..."
	GPIO.wait_for_edge(18, GPIO.FALLING)

	# Record for minimum of 500ms
	timer = time.time() + 0.5

	print "recording ..."
	while (not GPIO.input(18)) or (timer > time.time()):
		size, data = rx.read()
		if size:
			buf += data
		time.sleep(.001)

	rx.close()
	print "analyzing ..."
	return buf

def debug_voice_playback(buf):
	# Setup alsa playback interface
	tx = alsaaudio.PCM(alsaaudio.PCM_PLAYBACK, alsaaudio.PCM_NORMAL, 'sysdefault')
	tx.setchannels(1)
	tx.setrate(16000)
	tx.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	tx.setperiodsize(341)

	for i in range(0, len(buf)-CHUNK, CHUNK):
		tx.write(buf[i:i+CHUNK])
	tx.close()

def play(filename):
	filename = "audio/" + filename + ".wav"
	print "playing", filename

	f = wave.open(filename, 'rb')

	# Setup alsa playback interface
	tx = alsaaudio.PCM(alsaaudio.PCM_PLAYBACK, alsaaudio.PCM_NORMAL, 'sysdefault')
	tx.setchannels(f.getnchannels())
	tx.setrate(f.getframerate())

	# 8bit is unsigned in wav files
	if f.getsampwidth() == 1:
	    tx.setformat(alsaaudio.PCM_FORMAT_U8)
	# Otherwise we assume signed data, little endian
	elif f.getsampwidth() == 2:
	    tx.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	elif f.getsampwidth() == 3:
	    tx.setformat(alsaaudio.PCM_FORMAT_S24_LE)
	elif f.getsampwidth() == 4:
	    tx.setformat(alsaaudio.PCM_FORMAT_S32_LE)
	else:
	    raise ValueError('Unsupported format')

	tx.setperiodsize(PERIOD)

	data = f.readframes(PERIOD)
	while data:
		tx.write(data)
		data = f.readframes(PERIOD)

	tx.close()
	f.close()

def unknown():
	rand = random.randint(1,3)
	play("repeat_" + str(rand))

def confirm():
	x = ['confirm_1', 'confirm_2', 'confirm_3', 'confirm_4', 'confirm_5',
		'confirm_6', 'confirm_7', 'confirm_8', 'confirm_9',
		'confirm_10', 'activated']
	play(random.choice(x))

def say_time(short=0):
	pm = False
	clock = time.localtime()

	if not short:
		rand = random.randint(0,1)
		play("clock_time_" + str(rand))

	# Convert military to civilian time
	if (clock.tm_hour > 12):
		hour = clock.tm_hour - 12
		pm = True
	else:
		hour = clock.tm_hour

	play("num_" + str(hour))
	if (clock.tm_min):
		if (clock.tm_min < 10):
			play("num_0" + str(clock.tm_min))
		else:
			play("num_" + str(clock.tm_min))

		if pm:
			play("clock_pm")
		else:
			play("clock_am")
	else:
		play("clock_oclock")
	

def send_cmd(c):
	ser.write(c)

def set_lights(c):
	colormap = {	'RED': '3 255 0 0\n',
			'ORANGE': '3 255 127 0\n',
			'YELLOW': '3 255 255 0\n',
			'GREEN': '3 0 255 0\n',
			'BLUE': '3 0 0 255\n',
			'INDIGO': '3 75 0 130\n',
			'VIOLET': '3 143 0 255\n',
			'WHITE': '3 255 255 255\n' }

	if any(word in c for word in ['ON', 'DEFAULT']):
		send_cmd("4 127\n")
		send_cmd(colormap['RED'])
	elif 'OFF' in c:
		send_cmd("3 0 0 0\n")		

	elif 'RED' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['RED'])
	elif 'ORANGE' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['ORANGE'])
	elif 'YELLOW' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['YELLOW'])
	elif 'GREEN' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['GREEN'])
	elif 'BLUE' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['BLUE'])
	elif 'INDIGO' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['INDIGO'])
	elif 'VIOLET' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['VIOLET'])
	elif 'WHITE' in c:
		send_cmd("4 127\n")
		send_cmd(colormap['WHITE'])

	elif 'BRIGHTNESS' in c:
		val = wordtoi(c)
		if ((val >= 0) and (val <= 100)):
			val = scale(val, 0, 100, 0, 255)
			send_cmd('4 %s\n'%{val})
			confirm()
		else:
			unknown()


def wordtoi(c):
	if 'ZERO' in c:
		return 0
	elif 'TEN' in c:
		return 10
	elif 'TWENTY' in c:
		return 20
	elif 'THIRTY' in c:
		return 30
	elif 'FORTY' in c:
		return 40
	elif 'FIFTY' in c:
		return 50
	elif 'SIXTY' in c:
		return 60
	elif 'SEVENTY' in c:
		return 70
	elif 'EIGHTY' in c:
		return 80
	elif 'NINETY' in c:
		return 90
	elif 'ONE HUNDRED' in c:
		return 100
	else:
		return -1

def action_tree(c):
	# Unknown Request
	if c == '':
		unknown()

	# Respond to JUST "jarvis"
	elif c == 'JARVIS':
		x = ['title_u_s', 'listening_on_2', 'listening_on_3', 'listening_on_4',
			'listening_on_5', 'listening_on_6', 'listening_on_7']
		play(random.choice(x))
	
	# Open faceplate
	elif any(word in c for word in ['OPEN', 'UP']):
		send_cmd("1\n")
		confirm()

	# Close faceplate
	elif any(word in c for word in ['CLOSE', 'DOWN']):
		send_cmd("2\n")
		confirm()

	# What time is it
	elif 'TIME' in c:
		say_time()

	# Set lights to 'ROYGBIV'
	# Set colors ...
	# Set lights to 30 percent brightness
	elif any(word in c for word in ['LIGHTS', 'COLORS', 'BRIGHTNESS']):
		set_lights(c)
		confirm()

	elif 'PARTY' in c:
		send_cmd("5 1\n")
		confirm()

	elif 'EXWIFE' in c:
		if any(word in c for word in ['OPEN', 'ACTIVATE']):
			send_cmd("10 1\n")
		else:
			send_cmd("10 0\n")
		confirm()

	elif any(word in c for word in ['BOGEY', 'ENEMY', 'BATTLE']):
		send_cmd("2\n")
		send_cmd("10 1\n")
		play('clock_alarm_snooze_1')

	elif any(word in c for word in ['STAND DOWN', 'RELAX']):
		send_cmd("10 0\n")
		send_cmd("1\n")
		confirm()

	elif 'UNMUTE' in c:
		# This could hit the mute pin but it's acting strange
		GPIO.output(27, True)
		confirm()

	elif 'MUTE' in c:
		# This could hit the mute pin but it's acting strange
		GPIO.output(27, False)
		confirm()

	elif 'HACK' in c:
		play('accessed')
	
	elif 'DELETED PICTURES AND VIDEOS' in c:
		play('bd_deleted_scenes')
		play('bd_feature_0')

	elif 'CHARGE' in c:
		play('battery_charging_1')
		#TODO put charge up sound here instead of sleep
		time.sleep(3)
		play('battery_charged')
	
	elif 'SELF DESTRUCT' in c:
		play('activated')

	elif all(word in c for word in ['SELF DESTRUCT', 'THEIR']):
		play('self_destruct_1')

	elif 'CAMERA' in c:
		play('camera_on')

	elif any(word in c for word in ['LATE', 'PHOTO']):
		#TODO: alert_0 also works here
		play('clock_reminder_alert_1')

	elif 'HELP' in c:
		play('instructions')

	elif 'THANKS' in c:
		play('clock_late_1')
		# confirm_0, confirm_10, confirm_9

	elif any(word in c for word in ['SHUTDOWN', 'POWER OFF']):
		play('sleep_2')
		set_amp_gain(0)
		os.system('shutdown now -h')

	elif 'REBOOT' in c:
		play('sleep_2')
		set_amp_gain(0)
		os.system('reboot')

	elif 'HOT' in c:
		play('weather_hot_1')

	elif 'VOLUME' in c:
		vol = wordtoi(c)
		if ((vol >= 0) and (vol <= 100)):
			volume = vol
			set_amp_gain(scale(volume, 0, 100, 0, 63))
			confirm()
		else:
			unknown()

	# Unknown Request
	else:
		unknown()

def setup_amp():
	#Set gain to something
	set_amp_gain(volume)
	#bring shutdown high
	GPIO.setup(17, GPIO.OUT)
	GPIO.output(17, True)
	#bring mute high
	GPIO.setup(27, GPIO.OUT)
	GPIO.output(27, True)

def set_amp_gain(gain):
	if (gain < 0):
		gain = 0
	elif (gain > 63):
		gain = 63
	try:
		bus.write_byte(0x4B, gain)
	except Exception as ex:
		print("Exception [%s]" % (ex))

if __name__ == '__main__':
	global ser
	ser = serial.Serial('/dev/ttyACM0', 9600)

	random.seed(time.time())

	# Setup gpio 18 (pin 12) for Push-to-talk
	GPIO.setmode(GPIO.BCM)
	GPIO.setwarnings(False)
	GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_UP)

	# Setup pocketsphinx voice-to-text engine
	decoder = decoder_init()

	setup_amp()

	# Arduino code bug, clear out first command
	send_cmd(" ")

	# Open faceplate & close exwife
	send_cmd("1\n")
	time.sleep(1)
	send_cmd("10 0\n")

	# Jarvis coming online audio
	play('trs_help')
	say_time(short=1)

	while True:
		buf = get_audio()
		#debug_voice_playback(buf)
		command = decode(decoder, buf)
		print('Hypothesis ', command)
		action_tree(command)
		
