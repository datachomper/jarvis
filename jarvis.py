from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *
import thread
import pyaudio
import wave
import time

buf = []

class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

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

def audio_callback(in_data, frame_count, time_info, status):
	global buf
	buf.append(in_data)
	return (None, pyaudio.paContinue)

def wait_for_key():
	getch = _GetchUnix()
	while not getch():
		pass
	return 0

def get_audio(ad):
	print "Waiting for keypress"
	wait_for_key()

	stream = ad.open(format = pyaudio.paInt16,
			channels = 1,
			rate = 16000,
			input=True,
			output=False,
			frames_per_buffer=1024,
			stream_callback=audio_callback)

	print "recording ..."
	stream.start_stream()
	wait_for_key()
	stream.stop_stream()
	stream.close()
	ad.terminate()
	print "finished recording"

def get_audio_file():
	stream = file('/tmp/voice_test.wav', 'rb')
	stream.seek(44)
	buf = stream.read()
	return buf

if __name__ == '__main__':
	decoder = decoder_init()
	ad = pyaudio.PyAudio()
	get_audio(ad)
	print type(buf).__name__
	print type(get_audio_file()).__name__
	print 'Hypothesis: ', decode(decoder, buf)
