#ifndef MIXER_SDL_H
#define MIXER_SDL_H

#include "mixer.h"

Mixer* MXR_InitSDLOutput(float rate);
/* Prepares for audio output using SDL.
   Call SDL_Init(SDL_INIT_AUDIO) beforehand; this just
   sets up the audio subsystem.
   Returns a mixer interface, or NULL on failure. */

void MXR_CloseSDLOutput(Mixer* mixer);
/* Ends audio output through SDL. Frees the mixer's
   resources. */

void MXR_BeginSDLOutput();
/* Begins the SDL output stream. */

#endif
