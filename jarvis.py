from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *
import thread
import alsaaudio
import time

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
	return decoder.hyp().hypstr

def input_thread(L):
	raw_input()
	L.append(None)

def wait_for_key():
	L = []
	thread.start_new_thread(input_thread, (L,))
	while not L:
		pass

def get_audio():
	buf = ''
	print "Waiting for keypress"
	wait_for_key()

    	rx = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NONBLOCK, 'sysdefault')
	rx.setchannels(1)
	rx.setrate(16000)
	rx.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	rx.setperiodsize(160)

	print "recording ..."
	L = []
	thread.start_new_thread(input_thread, (L,))
	while not L:
		size, data = rx.read()
		if size:
			buf += data
			time.sleep(.001)
	print "finished recording"
	return buf

def get_audio_file():
	stream = file('/tmp/voice_test.wav', 'rb')
	stream.seek(44)
	buf = stream.read()
	return buf

if __name__ == '__main__':
	decoder = decoder_init()
	buf = get_audio()
	print('Hypothesis', decode(decoder, buf))
