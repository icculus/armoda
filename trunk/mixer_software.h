#ifndef MIXER_SOFTWARE_H
#define MIXER_SOFTWARE_H

#include "mixer.h"

typedef unsigned int (*SoftMixerOutputProc)(void* user, float* stereo_out, unsigned int frames);

Mixer* MXR_InitSoftMixer(float rate, SoftMixerOutputProc output, void* user);
/* Allocates and initializes a new software mixer structure.
   This mixer will output audio through the given output function,
   passing the user pointer as the first argument. */

void MXR_FreeSoftMixer(Mixer* mixer);
/* Frees a software mixer. */

#endif
