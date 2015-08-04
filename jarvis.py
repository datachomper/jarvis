from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *
import thread
import alsaaudio
import time
import curses

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

def wait_for_key():
	getch = _GetchUnix()
	while not getch():
		pass
	return 0

def get_audio(stdscr):
	#buf = bytearray()
	buf = ''
	print "Waiting for keypress"
	wait_for_key()

    	rx = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NONBLOCK, 'sysdefault')
	rx.setchannels(1)
	rx.setrate(16000)
	rx.setformat(alsaaudio.PCM_FORMAT_S16_LE)
	rx.setperiodsize(160)

	fp = open('/tmp/test.wav', 'wb')	
	print "recording ..."
	stdscr.nodelay(1)
	while (stdscr.getch() == -1):
		size, data = rx.read()
		if size:
			#fp.write(data)
			#buf.append(data)
			buf += data
			time.sleep(.001)
	print "finished recording"
	return buf

def get_audio_file():
	stream = file('/tmp/voice_test.wav', 'rb')
	stream.seek(44)
	buf = stream.read()
	return buf

def main(stdscr):
	decoder = decoder_init()
	buf = get_audio(stdscr)
	stdscr.addstr(decode(decoder, buf))
	stdscr.refresh()
	wait_for_key()

if __name__ == '__main__':
	curses.wrapper(main)

