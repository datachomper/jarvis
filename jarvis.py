#!/usr/bin/python

from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *
import alsaaudio
import time
import RPi.GPIO as GPIO
import random
import wave

button_pressed = False

# For some reason the period size is stuck on this number
PERIOD = 341
FRAME_SIZE = 2
CHUNK = PERIOD * FRAME_SIZE


def decoder_init():
	hmdir = '/usr/local/share/pocketsphinx/model/en-us/en-us'
	dictd = '/home/pi/sphinx_generated_dic/jarvis_v2.dic'
	lmdir = '/home/pi/sphinx_generated_dic/jarvis_v2.lm'

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

def say_time():
	clock = time.localtime()

	rand = random.randint(1,2)
	play("clock_time_" + rand)
	play("num_" + clock.tm_hour)
	play("num_" + clock.tm_min)
	play("clock_oclock")
	

def action_tree(c):
	# Unknown Request
	if c == '':
		unknown()

	# Respond to just "jarvis"
	elif c == 'JARVIS':
		x = ['title_u_s', 'listening_on_2', 'listening_on_3', 'listening_on_4',
			'listening_on_5', 'listening_on_6', 'listening_on_7']
		play(random.choice(x))
	
	# Open faceplate
	elif c.find('OPEN'):
		confirm()

	# Close faceplate
	elif c.find('CLOSE'):
		confirm()

	elif c.find('TIME'):
		say_time()

	# Unknown Request
	else:
		unknown()

if __name__ == '__main__':
	random.seed(time.time())

	# Setup button 18 for Push-to-talk
	GPIO.setmode(GPIO.BCM)
	GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_UP)

	# Setup pocketsphinx voice-to-text engine
	decoder = decoder_init()

	while True:
		buf = get_audio()
		#debug_voice_playback(buf)
		command = decode(decoder, buf)
		print('Hypothesis ', command)
		action_tree(command)
		
