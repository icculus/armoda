#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mixer_software.h"
#include "osutil.h"

#define FIXED_BITS 16

typedef struct SoftMixerPatch {

    int allocated;
    /* 0 if free, 1 if in use */
    
    float *samples;
    /* sample data */

    unsigned int length;
    /* number of samples */

} SoftMixerPatch;


typedef struct SoftMixerVoice {
    
    int allocated;
    /* 0 if free, 1 if in use */

    int patch;
    /* index of patch */

    float volume;
    /* mixing volume, 0..1 */

    float rate;
    /* playback rate, in Hz */

    float balance;
    /* stereo balance, -1..1 */

    unsigned int loop_pos;
    /* loop position */

    unsigned int loop_len;
    /* loop length */

    int triggered;
    /* 0 if stopped, 1 if playing */    

    unsigned int pos_fp;
    /* playback position -- FIXED POINT */

    unsigned int rate_fp;
    /* playback rate -- FIXED POINT */

    unsigned int play_to_fp;
    /* play to point -- FIXED POINT */

    int looped;
    /* loop enabled */

} SoftMixerVoice;

typedef struct SoftMixerPrivData {

    SoftMixerOutputProc output;
    /* Output callback. */

    void* user;
    /* User data for output callback. */

    float rate;
    /* Output rate. */
    
    SoftMixerVoice *voices;
    unsigned int num_voices;
    /* Array of voices. num_voices is the size of the array, not the number
       of currently active voices. */

    SoftMixerPatch *patches;
    unsigned int num_patches;
    /* Array of patches. */

} SoftMixerPrivData;

#define PRIV(mixer) ((SoftMixerPrivData*)mixer->private)



int LoadPatch(struct Mixer* mixer, float* samples, unsigned int length)
{
    unsigned int p, i;
    SoftMixerPrivData* data = PRIV(mixer);

    /* Search for a previously freed voice we can reuse. */
    for (p = 0; p < data->num_patches; p++) {
	if (data->patches[p].allocated == 0)
	    break;
    }

    /* Do we need to allocate another? */
    if (p == data->num_patches) {
	SoftMixerPatch* tmp = (SoftMixerPatch*)realloc(data->patches, sizeof (SoftMixerPatch) * (data->num_patches + 1));
	if (tmp == NULL) {
	    fprintf(stderr, "Software mixer: unable to realloc patches\n");
	    return -1;
	}	
	data->patches = tmp;
	data->num_patches++;
    }

    /* Prepare the patch. */
    memset(&data->patches[p], 0, sizeof (SoftMixerPatch));

    data->patches[p].allocated = 1;

    /* Copy samples over. This would be a good place to perform
       filtering, oversampling, etc. */
    data->patches[p].samples = (float *)malloc(length * sizeof (float));
    if (data->patches[p].samples == NULL) {
	fprintf(stderr, "Software mixer: unable to alloc data for patch\n");
	data->patches[p].allocated = 0;
	return -1;
    }
    for (i = 0; i < length; i++) {
	data->patches[p].samples[i] = samples[i];
    }
    data->patches[p].length = length;

    return p;
}


int UnloadPatch(struct Mixer* mixer, int patch)
{
    SoftMixerPrivData* data = PRIV(mixer);

    data->patches[patch].allocated = 0;
    free(data->patches[patch].samples);
    data->patches[patch].samples = NULL;

    return 0;
}


static int AllocVoice(Mixer* mixer, int priority)
{
    unsigned int v;
    SoftMixerPrivData* data = PRIV(mixer);

    /* Search for a previously freed voice we can reuse. */
    for (v = 0; v < data->num_voices; v++) {
	if (data->voices[v].allocated == 0)
	    break;
    }

    /* Do we need to allocate another? */
    if (v == data->num_voices) {
	SoftMixerVoice* tmp = (SoftMixerVoice*)realloc(data->voices, sizeof (SoftMixerVoice) * (data->num_voices + 1));
	if (tmp == NULL) {
	    fprintf(stderr, "Software mixer: unable to realloc voices\n");
	    return -1;
	}	
	data->voices = tmp;
	data->num_voices++;
    }

    /* Prepare the voice. */
    memset(&data->voices[v], 0, sizeof (SoftMixerVoice));

    data->voices[v].allocated = 1;
    data->voices[v].patch = -1;
    data->voices[v].volume = 0.0;
    data->voices[v].rate = 0.0;
    data->voices[v].balance = 0.0;
    data->voices[v].triggered = 0;
    data->voices[v].pos_fp = 0;
    data->voices[v].loop_pos = 0;
    data->voices[v].looped = 0;

    return v;
}


static int FreeVoice(Mixer* mixer, int voice)
{
    SoftMixerPrivData* data = PRIV(mixer);
    
    data->voices[voice].triggered = 0;
    data->voices[voice].patch = -1;
    data->voices[voice].allocated = 0;
    
    return 0;
}


static int IsVoiceLive(Mixer* mixer, int voice)
{
    return 1;  /* unlimited voices, so we never evict them */
}


static int SetVoicePatch(Mixer* mixer, int voice, int patch)
{
    SoftMixerPrivData* data = PRIV(mixer);

    data->voices[voice].patch = patch;
    data->voices[voice].volume = 0.0;
    data->voices[voice].rate = 0.0;
    data->voices[voice].balance = 0.0;
    data->voices[voice].triggered = 0;
    data->voices[voice].pos_fp = 0;
    data->voices[voice].looped = 0;
    data->voices[voice].rate_fp = 0;
    data->voices[voice].play_to_fp = data->patches[patch].length << FIXED_BITS;

    return 0;
}


static int SetVoiceVolume(Mixer* mixer, int voice, float volume)
{
    SoftMixerPrivData* data = PRIV(mixer);

    CLAMP(volume, 0.0, 1.0);
    data->voices[voice].volume = volume;

    return 0;
}


static int SetVoiceRate(Mixer* mixer, int voice, float rate)
{
    SoftMixerPrivData* data = PRIV(mixer);

    LOWER_CLAMP(rate, 0.0);
    rate /= data->rate;
    data->voices[voice].rate = rate;
    data->voices[voice].rate_fp = (unsigned int)((float)rate * (float)((unsigned int)1 << FIXED_BITS));
    
    return 0;
}


static int SetVoiceLoopPos(Mixer* mixer, int voice, unsigned int loop_start, unsigned int loop_len)
{
    SoftMixerPrivData* data = PRIV(mixer);

    data->voices[voice].loop_pos = loop_start;
    data->voices[voice].loop_len = loop_len;

    return 0;
}


static int SetVoiceLoopMode(Mixer* mixer, int voice, int enabled)
{
    SoftMixerPrivData* data = PRIV(mixer);

    data->voices[voice].looped = (enabled ? 1 : 0);
    
    return 0;
}


static int SetVoicePos(Mixer* mixer, int voice, unsigned int pos)
{
    SoftMixerPrivData* data = PRIV(mixer);
    
    data->voices[voice].pos_fp = pos << FIXED_BITS;

    return 0;
}


static int SetVoiceBalance(Mixer* mixer, int voice, float panning)
{
    SoftMixerPrivData* data = PRIV(mixer);

    CLAMP(panning, -1.0, 1.0);

    data->voices[voice].balance = panning;

    return 0;
}


static int TriggerVoice(Mixer* mixer, int voice)
{
    SoftMixerPrivData* data = PRIV(mixer);

    assert(data->voices[voice].patch >= 0);
    assert(data->patches[data->voices[voice].patch].allocated);

    data->voices[voice].triggered = 1;
    data->voices[voice].pos_fp = 0;
    data->voices[voice].play_to_fp = data->patches[data->voices[voice].patch].length << FIXED_BITS;

    return 0;
}


static int StopVoice(Mixer* mixer, int voice)
{
    SoftMixerPrivData* data = PRIV(mixer);

    data->voices[voice].triggered = 0;
    
    return 0;
}


static int Render(Mixer* mixer, unsigned int frames, float divisor)
{
    float* stereo_out;
    unsigned int v;
    unsigned int frame;
    SoftMixerPrivData* data = PRIV(mixer);
    unsigned int written;

    /* Allocate a mixing buffer.
       malloc generally isn't too expensive, so I'm not
       particularly concerned about this. */
    stereo_out = (float*)mallocsafe(frames * 2 * sizeof (float));
    memset(stereo_out, 0, frames * 2 * sizeof (float));
    
    for (v = 0; v < data->num_voices; v++) {

	if (data->voices[v].allocated
	    && data->voices[v].triggered
	    && data->voices[v].patch >= 0) {
	    
	    float left_vol, right_vol;
	    float panning = (data->voices[v].balance + 0.5) / 2.0;
	    float *out = stereo_out;
	    float *samples = data->patches[data->voices[v].patch].samples;
	    unsigned int pos_fp = data->voices[v].pos_fp;
	    unsigned int play_to_fp = data->voices[v].play_to_fp;

	    /* Calculate left and right volumes. */
	    left_vol = data->voices[v].volume * (1.0 - panning) / divisor;
	    right_vol = data->voices[v].volume * panning / divisor;

	    for (frame = 0; frame < frames; frame++) {
		float sample;

		if (pos_fp >= play_to_fp) {
		    if (data->voices[v].looped && data->voices[v].loop_len > 0) {
			/* Jump to loop point. */
			pos_fp = data->voices[v].loop_pos << FIXED_BITS;
			play_to_fp = (data->voices[v].loop_pos + data->voices[v].loop_len) << FIXED_BITS;
			UPPER_CLAMP(pos_fp, (data->patches[data->voices[v].patch].length-1) << FIXED_BITS);
			UPPER_CLAMP(play_to_fp, data->patches[data->voices[v].patch].length << FIXED_BITS);
		    } else {
			/* Not looped; end playback. */
			pos_fp = 0;
			data->voices[v].triggered = 0;
			break;
		    }
		}

		/* Fetch the next sample. */
		sample = samples[pos_fp >> FIXED_BITS];

		/* Update the playback position. */
		pos_fp += data->voices[v].rate_fp;

		/* Accumulate the sample into the output buffer. */
		*out++ += left_vol * sample;
		*out++ += right_vol * sample;
	    }

	    data->voices[v].pos_fp = pos_fp;
	    data->voices[v].play_to_fp = play_to_fp;

	}
    }

    /* Write the data. */
    written = 0;
    while (written < frames * 2) {
	unsigned int amt;
	amt = data->output(data->user,
			   stereo_out + written,
			   frames * 2 - written);
	written += amt;
    }
    
    /* Free the mixing buffer. */
    free(stereo_out);

    return 0;
}


Mixer* MXR_InitSoftMixer(float rate, SoftMixerOutputProc output, void* user)
{
    Mixer* mixer;
    
    mixer = (Mixer*)mallocsafe(sizeof (Mixer));
    memset(mixer, 0, sizeof (Mixer));
    mixer->private = mallocsafe(sizeof (SoftMixerPrivData));
    memset(mixer->private, 0, sizeof (SoftMixerPrivData));
    PRIV(mixer)->output = output;
    PRIV(mixer)->user = user;
    PRIV(mixer)->rate = rate;

    mixer->Render = Render;
    mixer->LoadPatch = LoadPatch;
    mixer->UnloadPatch = UnloadPatch;
    mixer->AllocVoice = AllocVoice;
    mixer->FreeVoice = FreeVoice;
    mixer->IsVoiceLive = IsVoiceLive;
    mixer->SetVoicePatch = SetVoicePatch;
    mixer->SetVoiceVolume = SetVoiceVolume;
    mixer->SetVoiceRate = SetVoiceRate;
    mixer->SetVoiceLoopPos = SetVoiceLoopPos;
    mixer->SetVoiceLoopMode = SetVoiceLoopMode;
    mixer->SetVoicePos = SetVoicePos;
    mixer->SetVoiceBalance = SetVoiceBalance;
    mixer->TriggerVoice = TriggerVoice;
    mixer->StopVoice = StopVoice;

    return mixer;
}


void MXR_FreeSoftMixer(Mixer* mixer)
{
    SoftMixerPrivData* data = PRIV(mixer);
    unsigned int i;
    
    for (i = 0; i < data->num_patches; i++) {
	if (data->patches[i].allocated)
	    UnloadPatch(mixer, i);
    }
    for (i = 0; i < data->num_voices; i++) {
	if (data->voices[i].allocated)
	    FreeVoice(mixer, i);
    }
    free(data->patches);
    free(data->voices);

    free(mixer->private);
    free(mixer);
}

