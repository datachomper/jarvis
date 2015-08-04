#include <alsa/asoundlib.h>
#include <stdio.h>

static snd_pcm_t *handle;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
static int open_mode = 0;

static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams, rhwparams;

static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);

static snd_pcm_uframes_t chunk_size = 0;
static u_char *audiobuf = NULL;

static void capture() {
}

int main(int argc, char *argv[])
{
	int err;
	char *pcm_name = "default";
	stream = SND_PCM_STREAM_CAPTURE;

	err = snd_pcm_open(&handle, pcm_name, stream, open_mode);
	if (err < 0) {
		printf("audio open error: %s", snd_strerror(err));
		return 1;
	}

	chunk_size = 1024;
	rhwparams.format = SND_PCM_FORMAT_S16_LE;
	rhwparams.rate = 16000;
	rhwparams.channels = 1;
	hwparams = rhwparams;

	audiobuf = (u_char *)malloc(1024);
	if (audiobuf == NULL) {
		printf("not enough memory");
		return 1;
	}

	writei_func = snd_pcm_writei;
	readi_func = snd_pcm_readi;
	writen_func = snd_pcm_writen;
	readn_func = snd_pcm_readn;

	capture();

	snd_pcm_close(handle);
	handle = NULL;
	free(audiobuf);
      __end:
	//snd_output_close(log);
	//snd_config_update_free_global();
	return 0;
}
