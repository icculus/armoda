#include <SDL/SDL.h>
#include <stdlib.h>
#include "mixer_sdl.h"
#include "mixer_software.h"
#include "osutil.h"

#define OUTPUT_BUF_SAMPLES (1024 * 128)

static float sdl_output[OUTPUT_BUF_SAMPLES];
static int sdl_output_put = 0;
static int sdl_output_get = 0;
static int sdl_output_avail = 0;
static int sdl_output_finished = 0;

void ResetOutput()
{
    int i;
    for (i = 0; i < OUTPUT_BUF_SAMPLES; i++)
	sdl_output[i] = 0.0;
    sdl_output_avail = OUTPUT_BUF_SAMPLES;
}

static void MixerProc(void *dummy, Uint8 *audio, int length)
{
    int i;
    int amt = length;

    Sint16 *out = (Sint16 *)audio;
    if (sdl_output_avail < length / 2) {
	memset(out, 0, length);
	amt = sdl_output_avail * 2;
	if (sdl_output_finished) {
	    if (sdl_output_avail == 0)
		return;
	} else {
	    printf("Buffer underrun in Armoda SDL mixer! Wanted %i bytes, had %i.\n", length, sdl_output_avail);
	}
    }
    for (i = 0; i < amt / 2; i++) {
	Sint16 sample = (Sint16)((float)32767.0 * sdl_output[sdl_output_get]);

	out[i] = sample;

	sdl_output_get++;
	sdl_output_avail--;
	if (sdl_output_get >= OUTPUT_BUF_SAMPLES)
	    sdl_output_get = 0;
    }
}

static void WaitBufferReady()
{
    int a;
    for (;;) {
	SDL_LockAudio();
	a = sdl_output_avail;
	SDL_UnlockAudio();
	if (sdl_output_avail < OUTPUT_BUF_SAMPLES)
	    break;
	SDL_Delay(300);
    }
}

static void WaitBufferEmpty()
{
    int a;
    for (;;) {
	SDL_LockAudio();
	a = sdl_output_avail;
	SDL_UnlockAudio();
	if (sdl_output_avail == 0)
	    break;
	SDL_Delay(100);
    }
}

static unsigned int WriteOutput(void* user, float *data, unsigned int amt)
{
    unsigned int avail, ttl, i;
    WaitBufferReady();
    SDL_LockAudio();
    avail = OUTPUT_BUF_SAMPLES - sdl_output_avail;
    ttl = (avail < amt ? avail : amt);
    for (i = 0; i < ttl; i++) {
	sdl_output[sdl_output_put] = data[i];
	sdl_output_put++;
	sdl_output_avail++;
	if (sdl_output_put >= OUTPUT_BUF_SAMPLES)
	    sdl_output_put = 0;
    }
    SDL_UnlockAudio();

    return ttl;
}

Mixer* MXR_InitSDLOutput(float rate)
{
    Mixer* mixer;
    SDL_AudioSpec fmt;

    fmt.freq = rate;
    fmt.format = AUDIO_S16SYS;
    fmt.samples = 16384;
    fmt.channels = 2;
    fmt.callback = MixerProc;
    fmt.userdata = NULL;

    if (SDL_OpenAudio(&fmt, NULL) < 0) {
	fprintf(stderr, "SDL mixer backend: unable to open SDL audio: %s\n", SDL_GetError());
	return NULL;
    }

    atexit(SDL_CloseAudio);

    mixer = MXR_InitSoftMixer(rate, WriteOutput, NULL);
    if (mixer == NULL) {
	fprintf(stderr, "SDL mixer backend: unable to create software mixer.\n");
	SDL_CloseAudio();
	return NULL;
    }

    ResetOutput();

    return mixer;
}

void MXR_CloseSDLOutput(Mixer* mixer)
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    free(mixer->private);
    free(mixer);
}

void MXR_BeginSDLOutput()
{
    SDL_PauseAudio(0);
}
