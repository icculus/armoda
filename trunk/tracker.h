#ifndef TRACKER_H
#define TRACKER_H

#include "commands.h"
#include "osutil.h"
#include "mixer.h"

typedef struct ARM_Channel {
    struct ARM_Sample* sample;
    float period;
    float c4spd;
    float volume;
    int cut_channel;  /* if > 0, decremented every tick. When zero, volume set to 0. */
    int delay_trigger;
    int retrigger;
    int patternloop_pos;
    int patternloop_count;
    int panning;
    int voice;          /* voice number on mixer */

    struct ARM_MOD_VibratoState vibrato_state;
    struct ARM_MOD_TremoloState tremolo_state;
    struct ARM_MOD_VolslideState volslide_state;
    struct ARM_MOD_ArpeggioState arpeggio_state;
    struct ARM_MOD_PeriodslideState periodslide_state;
    struct ARM_MOD_RetrigState retrig_state;
    struct ARM_MOD_TremorState tremor_state;

    struct ARM_Command command;

    const char *command_name;

} ARM_Channel;

typedef struct ARM_Module {
    char *title;
    int num_channels;
    int num_samples;
    struct ARM_Sample *samples;
    int num_order;
    int *order;
    int num_patterns;
    struct ARM_Pattern *patterns;
    int *default_pan;  /* NOTE: -1 = channel disabled */

    uint32_t flags; /* flag constants defined below */

    int global_volume;
    int master_volume;

    int initial_speed;
    int initial_bpm;
} ARM_Module;

typedef struct ARM_Tracker {
    int mixrate;
    struct ARM_Module *mod;
    int num_channels;
    struct ARM_Channel *channels;
    int patternbreak;    /* if >= 0, after all ticks for current row, go to this row on the next pattern */
    int goto_pattern;    /* if >= 0, after all ticks for current row, go to row 0 on this pattern */
    int order;
    int done;
    int pattern;
    int pos;
    int tick;
    int patterndelay;
    int speed;      /* ticks per division */
    int bpm;        /* beats per minute */
    int frames_per_tick;
    Mixer* mixer;
} ARM_Tracker;

typedef struct ARM_Sample {
    char *name;
    char *filename;
    unsigned int length;    /* length of sample in frames */
    float volume;           /* default volume */
    int repeat_ofs;         /* repeat offset in bytes */
    int repeat_len;         /* repeat length in bytes */
    int repeat_enabled;     /* repeat? */
    int c4spd;              /* base period of this sample */
    float *data;            /* floating point sample data */
    int patch;              /* mixer patch id */
} ARM_Sample;

typedef struct ARM_Pattern {
    int rows;
    int cols;
    struct ARM_Note **notes;
} ARM_Pattern;

typedef struct ARM_Note {
    struct ARM_Sample* sample;
    float volume;
    float period;
    int trigger;
    struct ARM_Command cmd;
} ARM_Note;

#define MODULE_FLAG_VOLSLIDE_ON_FIRST_TICK 1   /* default is to only slide on non-row ticks */
#define MODULE_FLAG_ENFORCE_AMIGA_LIMITS   2
#define MODULE_FLAG_KILL_SILENT_LOOPS      4   /* kill loops with 0 volume */


void ARM_ResetChannel(ARM_Tracker* player, int c);
/* Halts the given channel. */

void ARM_StartPattern(ARM_Tracker* player, int order, int pos);
/* Sets the playback position to the given order and row. */

float ARM_CalcSemitones(float period, float semitones);
/* Calculates the period value that is the given number of semitones above or below
   the given period. */

void ARM_TriggerChannel(ARM_Tracker* player, int channel, ARM_Sample* sample, float period, float volume);
/* Triggers playback on a channel. */

void ARM_InitTracker(ARM_Tracker* player, ARM_Module* mod, Mixer* mixer, int mixrate);
/* Prepares the tracker for playback. Installs patches, sets up playback variables, etc. */

void ARM_RenderOneTick(ARM_Tracker* player, float mix_divisor);
/* Renders one tick of music to the mixer. */

void ARM_AdvancePosition(ARM_Tracker* player);
/* Advances to the next position in the song.
   During normal playback, call this after each call to ARM_RenderOneTick. */

void ARM_FreeTrackerData(ARM_Tracker* player);
/* Deinitializes the tracker. Frees all data, unloads patches, etc. */

#include "module.h"
#include "pattern.h"

#endif
