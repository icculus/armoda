/*
 * mod2c test frame.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <SDL/SDL.h>
#include "tracker.h"
#include "mixer_sdl.h"
#include "deflorat.mod.h"

#define RATE 44100

int main(int argc, char *argv[])
{
    ARM_Module mod;
    ARM_Tracker player;
    Mixer* mixer;

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	printf("Unable to initialize SDL: %s\n", SDL_GetError());
	return 1;
    }

    atexit(SDL_Quit);

    mixer = MXR_InitSDLOutput((float)RATE);
    if (mixer == NULL) {
	printf("Unable to initialize mixer.\n");
	return 1;
    }

    if (LoadEmbeddedModule_test(&mod) < 0) {
	printf("Internal error.\n");
	return 1;
    }

    ARM_InitTracker(&player, &mod, mixer, RATE);
    MXR_BeginSDLOutput();

    while (!player.done) {
	
	ARM_RenderOneTick(&player, (float)player.num_channels);
	ARM_AdvancePosition(&player);
	
    }
	
    ARM_FreeTrackerData(&player);
    ARM_FreeModuleData(&mod);
    
    SDL_Delay(2000);

    MXR_CloseSDLOutput(mixer);
    
    return 0;
}
