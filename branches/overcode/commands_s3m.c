#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "commands.h"
#include "tracker.h"
#include "commands_s3m.h"
#include "commands_mod.h"

#define CHAN player->channels[c]

void command_s3m_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    int amt;
    if (arg1 == 0 && arg2 == 0) {
	arg1 = CHAN.volslide_state.last_valid_type_arg;
	arg2 = CHAN.volslide_state.last_valid_direction_arg;
    }
    CHAN.volslide_state.last_valid_type_arg = arg1;
    CHAN.volslide_state.last_valid_direction_arg = arg2;
    if (arg2 == 0x0F) {
	CHAN.command.callbacks = &command_mod_fine_volslide_callbacks;
	amt = arg1;
    } else if (arg1 == 0x0F) {
	CHAN.command.callbacks = &command_mod_fine_volslide_callbacks;
	amt = -arg2;
    } else if (arg2 == 0) {
	CHAN.command.callbacks = &command_mod_volslide_callbacks;
	amt = arg1;
    } else if (arg1 == 0) {
	CHAN.command.callbacks = &command_mod_volslide_callbacks;
	amt = -arg2;
    } else {
	CHAN.command.callbacks = &command_null_callbacks;
	amt = 0;
    }
    if (CHAN.command.callbacks->init_proc != NULL)
	CHAN.command.callbacks->init_proc(player, c, amt, 0);
}

ARM_CommandType command_s3m_volslide_callbacks = { command_s3m_volslide_init, NULL, NULL, NULL, 0 };


void command_s3m_period_slide_up_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    int amt;

    if (arg1 == 0 && arg2 == 0) {
	arg1 = CHAN.periodslide_state.last_valid_delta_arg;
	arg2 = CHAN.periodslide_state.last_valid_limit_arg;
    }

    CHAN.periodslide_state.last_valid_delta_arg = arg1;
    CHAN.periodslide_state.last_valid_limit_arg = arg2;
    if (arg1 == 0x0F) {
	CHAN.command.callbacks = &command_mod_fine_period_slide_callbacks;
	amt = arg2 * 4;
    } else if (arg1 == 0x0E) {
	CHAN.command.callbacks = &command_mod_fine_period_slide_callbacks;
	amt = arg2;
    } else {
	CHAN.command.callbacks = &command_mod_period_slide_callbacks;
	amt = arg2 * 4;
    }
    CHAN.command.callbacks->init_proc(player, c, -amt, 0);
}

ARM_CommandType command_s3m_period_slide_up_callbacks = { command_s3m_period_slide_up_init, NULL, NULL, NULL, 0 };


void command_s3m_period_slide_down_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    int amt;

    if (arg1 == 0 && arg2 == 0) {
	arg1 = CHAN.periodslide_state.last_valid_delta_arg;
	arg2 = CHAN.periodslide_state.last_valid_limit_arg;
    }

    CHAN.periodslide_state.last_valid_delta_arg = arg1;
    CHAN.periodslide_state.last_valid_limit_arg = arg2;
    if (arg1 == 0x0F) {
	CHAN.command.callbacks = &command_mod_fine_period_slide_callbacks;
	amt = arg2 * 4;
    } else if (arg1 == 0x0E) {
	CHAN.command.callbacks = &command_mod_fine_period_slide_callbacks;
	amt = arg2;
    } else {
	CHAN.command.callbacks = &command_mod_period_slide_callbacks;
	amt = arg2 * 4;
    }
    CHAN.command.callbacks->init_proc(player, c, amt, 0);
}


ARM_CommandType command_s3m_period_slide_down_callbacks = { command_s3m_period_slide_down_init, NULL, NULL, NULL, 0 };


void command_s3m_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command.callbacks = &command_mod_vibrato_callbacks;
    command_mod_vibrato_callbacks.init_proc(player, c, arg1, arg2);
    CHAN.vibrato_state.retrig = 0;
}

ARM_CommandType command_s3m_vibrato_callbacks = { command_s3m_vibrato_init, NULL, NULL, NULL, 0 };


void command_s3m_tremor_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.tremor_state.on_ticks = arg1 + 1;
    CHAN.tremor_state.off_ticks = arg2 + 1;    
    CHAN.tremor_state.saved_volume = CHAN.volume;
    CHAN.tremor_state.counter = 0;
}

void command_s3m_tremor_tick(struct ARM_Tracker* player, int c)
{
    if (CHAN.tremor_state.counter < CHAN.tremor_state.on_ticks) {
	CHAN.volume = CHAN.tremor_state.saved_volume;
    } else {
	CHAN.volume = 0;
    }
    CHAN.tremor_state.counter++;
    if (CHAN.tremor_state.counter >= CHAN.tremor_state.on_ticks + CHAN.tremor_state.off_ticks) {
	CHAN.tremor_state.counter = 0;
    }
}

void command_s3m_tremor_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.volume = CHAN.tremor_state.saved_volume;
}

ARM_CommandType command_s3m_tremor_callbacks = { command_s3m_tremor_init, command_s3m_tremor_tick, NULL, command_s3m_tremor_cleanup, 0 };


void command_s3m_volslide_and_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    if (arg1 == 0) {
	arg1 = CHAN.volslide_state.last_valid_delta;
    }
    command_mod_volslide_callbacks.init_proc(player, c, arg1, 0);
    command_s3m_vibrato_init(player, c, CHAN.vibrato_state.speed, CHAN.vibrato_state.amplitude);
    CHAN.command_name = "volslide and vibrato";
}

ARM_CommandType command_s3m_volslide_and_vibrato_callbacks = { command_s3m_volslide_and_vibrato_init, command_mod_volslide_and_vibrato_tick, NULL, command_mod_volslide_and_vibrato_cleanup, 0 };


void command_s3m_period_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    if (arg1 != 0)
	arg2 = 0;
    else if (arg2 != 0)
	arg1 = 0;
    command_s3m_volslide_init(player, c, arg1, arg2);
    command_mod_slide_to_note_init(player, c, 0, 0);
    CHAN.command_name = "period and volume slide";

}

ARM_CommandType command_s3m_period_and_volslide_callbacks = { command_s3m_period_and_volslide_init, command_mod_period_and_volslide_tick, NULL, command_mod_period_and_volslide_cleanup, 1 };



void command_s3m_retrig_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "retrigger and volslide";
    CHAN.retrig_state.ticks_between = arg1;
    CHAN.retrig_state.volume_adjust = arg2;
}

ARM_CommandType command_s3m_retrig_and_volslide_callbacks = { command_s3m_retrig_and_volslide_init, command_mod_retrigger_tick, NULL, command_mod_retrigger_cleanup, 0 };



void command_s3m_tremolo_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command.callbacks = &command_mod_tremolo_callbacks;
    command_mod_tremolo_init(player, c, arg1, arg2);
    CHAN.tremolo_state.retrig = 0;
}

ARM_CommandType command_s3m_tremolo_callbacks = { command_s3m_tremolo_init, NULL, NULL, NULL, 0 };
