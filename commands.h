#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

struct ARM_Tracker;

typedef void (*ARM_CommandInitProc)(struct ARM_Tracker* player, int channel, int arg1, int arg2);
typedef void (*ARM_CommandTickProc)(struct ARM_Tracker* player, int channel);
typedef void (*ARM_CommandCleanupProc)(struct ARM_Tracker* player, int channel);

typedef struct ARM_CommandType {
    ARM_CommandInitProc init_proc;        /* executed during row initialization */
    ARM_CommandTickProc pre_tick_proc;    /* executed before a tick's samples are mixed */
    ARM_CommandTickProc post_tick_proc;   /* executed after a tick's samples are mixed */
    ARM_CommandCleanupProc cleanup_proc;  /* executed before the next row's initialization */
    uint8_t no_reset_sample;              /* if 1, do not reset the sample for this effect, even if one is given */
} ARM_CommandType;

typedef struct ARM_Command {
    int cmd;          /* map to callbacks with ARM_GetCallbacksForNum */
    int arg1, arg2;
} ARM_Command;

typedef struct ARM_MOD_VibratoState {
    int amplitude;
    int speed;
    int pos;
    int retrig;
    float period_shift;
    int enabled;
} ARM_MOD_VibratoState;

typedef struct ARM_MOD_TremoloState {
    int speed;
    int pos;
    int amplitude;
    int retrig;
    int enabled;
} ARM_MOD_TremoloState;

typedef struct ARM_MOD_VolslideState {
    float delta_per_tick;
    int last_valid_type_arg;
    int last_valid_direction_arg;
    int last_valid_delta;
} ARM_MOD_VolslideState;

typedef struct ARM_MOD_ArpeggioState {
    float saved_period;
    float period_x, period_y;
} ARM_MOD_ArpeggioState;

typedef struct ARM_MOD_PeriodslideState {
    float delta_per_tick;
    float limit;
    int last_valid_delta_arg;
    int last_valid_limit_arg;
} ARM_MOD_PeriodslideState;

typedef struct ARM_MOD_RetrigState {
    int ticks_between;
    int counter;
    int volume_adjust;
} ARM_MOD_RetrigState;

typedef struct ARM_MOD_TremorState {
    int on_ticks;
    int off_ticks;
    int saved_volume;
    int counter;
} ARM_MOD_TremorState;

extern ARM_CommandType command_null_callbacks;


ARM_CommandType* ARM_GetCallbacksForNum(unsigned int cmd);
/* Maps an integer to a command callback structure. */

unsigned int ARM_GetNumForCallbacks(ARM_CommandType* callbacks);
/* Maps a callback structure pointer to a command number. */

#endif
