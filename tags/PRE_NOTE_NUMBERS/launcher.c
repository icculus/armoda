/*
 * Simple command line front end for Armoda.
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

#define RATE 44100    /* FIXME: parameterize this */

static int skip_flag = 0;
static int exit_flag = 0;

static void SignalHandler(int sig)
{
    if (sig == SIGINT) {
	printf("Interrupt.\n");
	if (exit_flag == 1) {
	    printf("Forced exit.\n");
	    exit(1);
	} else if (skip_flag == 1) {
	    printf("Exit request.\n");
	    exit_flag = 1;
	} else {
	    printf("Next track.\n");
	    skip_flag = 1;
	}
    }
}

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
    int force_master_volume = -1;
    int force_global_volume = -1;
    int verbose = 0;
    int arg;

    if (argc < 2) {
	printf("You must supply at least one filename.\n");
	return 1;
    }

    signal(SIGINT, SignalHandler);

    /* CLEANUP: better argument processing system */
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
	} else if (!strcmp(argv[arg], "--info")) {
	    show_song_info = 1;
	    show_sample_info = 1;
	} else if (!strcmp(argv[arg], "--force-master")) {
	    force_master_volume = atoi(argv[arg+1]);
	    arg++;
	} else if (!strcmp(argv[arg], "--force-global")) {
	    force_global_volume = atoi(argv[arg+1]);
	    arg++;
	} else if (!strcmp(argv[arg], "--verbose")) {
	    verbose = 1;
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

	if (ARM_LoadModule(&mod, argv[arg]) < 0) {
	    printf("Error loading module '%s'.\n", argv[arg]);
	    continue;
	}

	ARM_InitTracker(&player, &mod, mixer, RATE);

	if (force_master_volume >= 0)
	    mod.master_volume = force_master_volume;
	if (force_global_volume >= 0)
	    mod.global_volume = force_global_volume;

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

	    ARM_RenderOneTick(&player, (float)player.num_channels);
	    ARM_AdvancePosition(&player);

	    if (exit_flag || skip_flag)
		break;

	}
	
	ARM_FreeTrackerData(&player);
	ARM_FreeModuleData(&mod);

	printf("End of module %s.\n", argv[arg]);

	skip_flag = 0;
	if (exit_flag)
	    break;

    }

    /* Ok, so this is cheeseball. */
    if (!exit_flag)
	SDL_Delay(1000);

    /* Stop SDL's audio output. */
    MXR_CloseSDLOutput(mixer);
    
    return 0;
}
