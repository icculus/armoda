/*
 * Simple command line front end for Armoda.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include "tracker.h"
#include "mixer_sdl.h"

#define RATE 44100    /* FIXME: parameterize this */

int main(int argc, char *argv[])
{
    ARM_Module mod;
    ARM_Tracker player;
    Mixer* mixer;
    int startord = 0;
    int startspeed = 0;
    int startbpm = 0;
    int silence = 0;
    int show_song_info = 0;
    int show_sample_info = 0;
    int arg;

    if (argc < 2) {
	printf("You must supply a filename.\n");
	return 1;
    }

    for (arg = 1; arg < argc-1; arg++) {
	if (!strcmp(argv[arg], "--order")) {
	    startord = atoi(argv[arg+1]);
	    printf("Starting at order %i.\n", startord);
	    arg++;
	} else if (!strcmp(argv[arg], "--speed")) {
	    startspeed = atoi(argv[arg+1]);
	    printf("Starting at speed %i.\n", startspeed);
	    arg++;
	} else if (!strcmp(argv[arg], "--bpm")) {
	    startbpm = atoi(argv[arg+1]);
	    printf("Starting at %i BPM.\n", startbpm);
	    arg++;
	} else if (!strcmp(argv[arg], "--songinfo")) {
	    show_song_info = 1;
	} else if (!strcmp(argv[arg], "--sampleinfo")) {
	    show_sample_info = 1;
	} else break;
    }

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

    for (; arg < argc; arg++) {

	char *ext;

	if (strlen(argv[arg]) < 3) {
	    printf("Bad filename %s\n", argv[arg]);
	    continue;
	}

	ext = argv[arg] + strlen(argv[arg]) - 3;

 	if (!strcasecmp(ext, "s3m")) {
  	    if (ARM_LoadModule_S3M(&mod, argv[arg]) < 0) {
  		printf("Unable to load Scream Tracker module %s\n", argv[arg]);
  		continue;
  	    }
	} else if (!strcasecmp(ext, "mod")) {
	    if (ARM_LoadModule_MOD(&mod, argv[arg]) < 0) {
		printf("Unable to load Protracker module '%s'.\n", argv[arg]);
		continue;
	    }
	} else {
	    printf("Unknown file extension '%s'.\n", ext);
	}

	ARM_InitTracker(&player, &mod, mixer, RATE);

	if (show_song_info) {
	    printf("+- Module info: %s\n", argv[arg]);
	    printf("| Title:      %s\n", mod.title);
	    printf("| Channels:   %i\n", mod.num_channels);
	    printf("| Samples:    %i\n", mod.num_samples);
	    printf("| Length:     %i patterns\n", mod.num_order);
	    printf("| Vol slides: %s\n", (mod.flags & MODULE_FLAG_VOLSLIDE_ON_FIRST_TICK ? "fast" : "normal"));
	    printf("\n");
	}

	if (show_sample_info) {
	    int s;
	    printf("+- Sample info: %s\n", argv[arg]);
	    printf("|         Length    C4SPD  Loop?  Name/Comments\n");
	    for (s = 0; s < mod.num_samples; s++) {		
		printf("| %3i:   %6i    %5i   %s    %s\n",
		       s,
		       mod.samples[s].length,
		       mod.samples[s].c4spd,
		       mod.samples[s].repeat_enabled ? "yes" : "no ",
		       mod.samples[s].name);
	    }
	    printf("\n");
	}

	if (startord != 0) {
	    ARM_StartPattern(&player, startord, 0);
	}

	if (startspeed != 0) {
	    player.speed = startspeed;
	}

	if (startbpm != 0) {
	    player.bpm = startbpm;
	}

	MXR_BeginSDLOutput();

	while (!player.done) {
	    int i, c;

	    ARM_RenderOneTick(&player, (float)player.num_channels);
	    ARM_AdvancePosition(&player);

	}
	
	ARM_FreeTrackerData(&player);
	ARM_FreeModuleData(&mod);

	printf("End of module %s.\n", argv[arg]);

    }

    /* Ok, so this is cheeseball. */
    SDL_Delay(1000);

    /* Stop SDL's audio output. */
    MXR_CloseSDLOutput(mixer);
    
    return 0;
}
