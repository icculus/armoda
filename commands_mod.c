#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "commands.h"
#include "commands_mod.h"
#include "tracker.h"

#define CHAN player->channels[c]

ARM_CommandType command_null_callbacks = { NULL, NULL, NULL, NULL, 0 };


float ARM_MOD_CalcVibrato(struct ARM_Tracker* player, int c)
{
    float amp;
    float pos;
    float val;

    if (player->channels[c].vibrato_state.enabled == 0)
	return 0;

    amp = (float)ARM_CalcSemitones(player->channels[c].period, (float)player->channels[c].vibrato_state.amplitude / 32.0) -
	player->channels[c].period;

    pos = (float)player->channels[c].vibrato_state.pos * 2.0 * 3.141592654 / 64.0;
    val = amp * sin(pos + 3.141592654);

    return val;
}

float ARM_MOD_CalcTremolo(struct ARM_Tracker* player, int c)
{
    float amp;
    float pos;
    float val;

    if (player->channels[c].tremolo_state.enabled == 0)
	return 0;

    amp = (float)player->channels[c].tremolo_state.amplitude / 16.0;
    pos = (float)player->channels[c].tremolo_state.pos * 2.0 * 3.141592654 / 32.0;
    val = amp * sin(pos + 3.141592654);

    return val;
}



/*
 *  ARPEGGIO
 */

void command_mod_arpeggio_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "arpeggio";
    CHAN.arpeggio_state.saved_period = CHAN.period;
    CHAN.arpeggio_state.period_x = ARM_CalcSemitones(
	CHAN.arpeggio_state.saved_period, arg1);
    CHAN.arpeggio_state.period_y = ARM_CalcSemitones(
	CHAN.arpeggio_state.saved_period, arg2);
}

void command_mod_arpeggio_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.period = CHAN.arpeggio_state.saved_period;
}

void command_mod_arpeggio_tick(struct ARM_Tracker* player, int c)
{
    if (player->tick % 3 == 0) {
	CHAN.period = CHAN.arpeggio_state.saved_period;
    } else if (player->tick % 2 == 1) {
	CHAN.period = CHAN.arpeggio_state.period_y;
    } else if (player->tick % 3 == 2) {
	CHAN.period = CHAN.arpeggio_state.period_x;
    }
}

ARM_CommandType command_mod_arpeggio_callbacks = { command_mod_arpeggio_init, command_mod_arpeggio_tick, NULL, command_mod_arpeggio_cleanup, 0 };



/*
 *  PERIOD SLIDE
 *  Correct for MOD effects 1, 2, except possibly for clamping, but this is likely OK.
 *  Update code correct for MOD effect 3.
 */

void command_mod_period_slide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{    
    CHAN.command_name = (arg1 < 0) ? "period up" : "period down";

    /* Period slide of 0 means use last speed. */
    if (arg1 == 0)
	arg1 = CHAN.periodslide_state.last_valid_delta_arg;
    else
	CHAN.periodslide_state.last_valid_delta_arg = arg1;

    CHAN.periodslide_state.delta_per_tick = (float)arg1 / 1712.0;
    CHAN.periodslide_state.limit = 0.0;
}

void command_mod_period_slide_tick(struct ARM_Tracker* player, int c)
{
    /* Never slide on tick 0. */
    if (player->tick > 0) {
	/* Tone portamento, or just a slide? */
	if (CHAN.periodslide_state.limit != 0.0) {
	    CHAN.period += CHAN.periodslide_state.delta_per_tick;
	    if (CHAN.periodslide_state.delta_per_tick < 0.0) {
		LOWER_CLAMP(CHAN.period, CHAN.periodslide_state.limit);
	    } else {
		UPPER_CLAMP(CHAN.period, CHAN.periodslide_state.limit);
	    }
	} else {
	    CHAN.period += CHAN.periodslide_state.delta_per_tick;
	}
    }
}

void command_mod_period_slide_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.periodslide_state.delta_per_tick = 0.0;
    CHAN.periodslide_state.limit = 0.0;
}

ARM_CommandType command_mod_period_slide_callbacks = { command_mod_period_slide_init, command_mod_period_slide_tick, NULL, command_mod_period_slide_cleanup, 0 };


/*
 * TONE PORTAMENTO
 */

void command_mod_slide_to_note_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "tone portamento";

    /* Load old args if necessary. */
    if (arg1 == 0)
	arg1 = CHAN.periodslide_state.last_valid_delta_arg;
    if (arg2 == 0)
	arg2 = CHAN.periodslide_state.last_valid_limit_arg;

    /* Set the limit. */
    CHAN.periodslide_state.limit = ARM_GetLogPeriodForNote(arg2);
    
    /* Set the rate. */
    if (CHAN.periodslide_state.limit < CHAN.period) {
	CHAN.periodslide_state.delta_per_tick = -(float)arg1 / 1712.0;
    } else {
	CHAN.periodslide_state.delta_per_tick = (float)arg1 / 1712.0;
    }

    /* Save this command. */
    CHAN.periodslide_state.last_valid_delta_arg = arg1;
    CHAN.periodslide_state.last_valid_limit_arg = arg2;
}

ARM_CommandType command_mod_slide_to_note_callbacks = { command_mod_slide_to_note_init, NULL, command_mod_period_slide_tick, command_mod_period_slide_cleanup, 1 };



/*
 * VIBRATO
 */

void command_mod_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "vibrato";

    /* Set speed. */
    if (arg1 != 0)
	CHAN.vibrato_state.speed = arg1;

    /* Set amplitude. */
    if (arg2 != 0)
	CHAN.vibrato_state.amplitude = arg2;

    /* Reset position. (MOD) */
    CHAN.vibrato_state.pos = 0;

    /* Mark vibrato enabled. */
    CHAN.vibrato_state.enabled = 1;

    /* Retrigger on each row. */
    CHAN.vibrato_state.retrig = 1;
}

void command_mod_vibrato_tick(struct ARM_Tracker* player, int c)
{
    CHAN.vibrato_state.pos += CHAN.vibrato_state.speed;
    CHAN.vibrato_state.period_shift = ARM_MOD_CalcVibrato(player, c);
}

void command_mod_vibrato_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.vibrato_state.enabled = 0;
}

ARM_CommandType command_mod_vibrato_callbacks = { command_mod_vibrato_init, NULL, command_mod_vibrato_tick, command_mod_vibrato_cleanup, 0 };


/*
 * PORTA + VOLUME SLIDE
 */

void command_mod_period_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    command_mod_volslide_init(player, c, arg1, arg2);
    command_mod_slide_to_note_init(player, c, 0, 0);
    CHAN.command_name = "period and volume slide";

}

void command_mod_period_and_volslide_tick(struct ARM_Tracker* player, int c)
{
    command_mod_volslide_tick(player, c);
    command_mod_period_slide_tick(player, c);
}

void command_mod_period_and_volslide_cleanup(struct ARM_Tracker* player, int c)
{
    command_mod_volslide_cleanup(player, c);
    command_mod_period_slide_cleanup(player, c);
}

ARM_CommandType command_mod_period_and_volslide_callbacks = { command_mod_period_and_volslide_init, command_mod_period_and_volslide_tick, NULL, command_mod_period_and_volslide_cleanup, 1 };



/* 
 * VIBRATO + VOLUME SLIDE
 */

void command_mod_volslide_and_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    command_mod_volslide_init(player, c, arg1, 0);
    command_mod_vibrato_init(player, c, CHAN.vibrato_state.speed, CHAN.vibrato_state.amplitude);
    CHAN.command_name = "volslide and vibrato";
}

void command_mod_volslide_and_vibrato_tick(struct ARM_Tracker* player, int c)
{
    command_mod_volslide_tick(player, c);
    command_mod_vibrato_tick(player, c);
}

void command_mod_volslide_and_vibrato_cleanup(struct ARM_Tracker* player, int c)
{
    command_mod_volslide_cleanup(player, c);
    command_mod_vibrato_cleanup(player, c);
}

ARM_CommandType command_mod_volslide_and_vibrato_callbacks = { command_mod_volslide_and_vibrato_init, command_mod_volslide_and_vibrato_tick, NULL, command_mod_volslide_and_vibrato_cleanup, 0 };


/*
 * TREMOLO
 */

void command_mod_tremolo_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "tremolo";
    if (arg1 != 0)
	CHAN.tremolo_state.amplitude = arg1;

    if (arg2 != 0)
	CHAN.tremolo_state.amplitude = arg2;
    
    CHAN.tremolo_state.enabled = 1;
}

void command_mod_tremolo_tick(struct ARM_Tracker* player, int c)
{
    CHAN.tremolo_state.pos += CHAN.tremolo_state.speed;    
}

void command_mod_tremolo_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.tremolo_state.enabled = 0;
}

ARM_CommandType command_mod_tremolo_callbacks = { command_mod_tremolo_init, NULL, command_mod_tremolo_tick, command_mod_tremolo_cleanup, 0 };


/*
 * SET OFFSET
 */
void command_mod_offset_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "sample offset";
    if (CHAN.sample != -1) {
	player->mixer->SetVoicePos(player->mixer,
				   CHAN.voice,
				   arg1);
    }
}

ARM_CommandType command_mod_offset_callbacks = { command_mod_offset_init, NULL, NULL, NULL, 0 };





/*
 * VOLUME SLIDE
 */

void command_mod_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    if (arg1 < 0)
	CHAN.command_name = "volume down";
    else
	CHAN.command_name = "volume up";
    CHAN.volslide_state.delta_per_tick = (float)arg1;
}

void command_mod_volslide_tick(struct ARM_Tracker* player, int c)
{
    /* Do not volslide before the first tick, unless requested.
       (S3M has a version compatibility option for this.) */
    if (player->tick > 0 || (player->mod->flags & MODULE_FLAG_VOLSLIDE_ON_FIRST_TICK)) {
	CHAN.volume += CHAN.volslide_state.delta_per_tick;
	CLAMP(CHAN.volume, 0.0, 64.0);
    }
}

void command_mod_volslide_cleanup(struct ARM_Tracker* player, int c)
{
    CHAN.volslide_state.delta_per_tick = 0.0;
}

ARM_CommandType command_mod_volslide_callbacks = { command_mod_volslide_init, command_mod_volslide_tick, NULL, command_mod_volslide_cleanup, 0 };




/*
 * PATTERN JUMP
 */

void command_mod_goto_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    if ((arg2 == -1) && (arg1 != -1)) {
	CHAN.command_name = "goto pattern";
	player->goto_pattern = arg1;
    } else if ((arg2 != -1) && (arg1 == -1)) {
	CHAN.command_name = "pattern break";
	player->patternbreak = arg2;
    }
}

ARM_CommandType command_mod_goto_callbacks = { command_mod_goto_init, NULL, NULL, NULL, 0 };




/*
 * SET VOLUME
 */

void command_mod_set_volume_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "channel volume";
    CHAN.volume = (float)arg1;
    CLAMP(CHAN.volume, 0.0, 64.0);
}

ARM_CommandType command_mod_set_volume_callbacks = { command_mod_set_volume_init, NULL, NULL, NULL, 0 };




/*
 * SET SPEED
 * Correct for MOD effect F, though I'm not entirely sure how to handle zero speed.
 */

void command_mod_set_speed_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "set speed";
    if (arg1 > 0)
	player->speed = arg1;
}

ARM_CommandType command_mod_set_speed_callbacks = { command_mod_set_speed_init, NULL, NULL, NULL, 0 };




/*
 * SET TEMPO
 */

void command_mod_set_tempo_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "set tempo";
    if (arg1 > 0)
	player->bpm = arg1;
    LOWER_CLAMP(player->bpm, 0x20);
}

ARM_CommandType command_mod_set_tempo_callbacks = { command_mod_set_tempo_init, NULL, NULL, NULL, 0 };



/*
 * FINE PORTA
 */

void command_mod_fine_period_slide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = (arg1 < 0) ? "fine period down" : "fine period up";
    CHAN.period += (float)arg1 / 1712.0;
}

ARM_CommandType command_mod_fine_period_slide_callbacks = { command_mod_fine_period_slide_init, NULL, NULL, NULL, 0 };




/*
 * SET C4SPD
 */
void command_mod_set_c4spd_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "set c4spd";
    CHAN.c4spd = arg1;
}

ARM_CommandType command_mod_set_c4spd_callbacks = { command_mod_set_c4spd_init, NULL, NULL, NULL, 0 };




/*
 * SET PATTERN LOOP
 */

void command_mod_set_pattern_loop_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "set pattern loop";
    CHAN.patternloop_pos = player->pos;
}

ARM_CommandType command_mod_set_pattern_loop_callbacks = { command_mod_set_pattern_loop_init, NULL, NULL, NULL, 0 };

void command_mod_do_pattern_loop_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "do pattern loop";
    if (CHAN.patternloop_count == 0) {
	CHAN.patternloop_count = -1;
	return;
    } else if (CHAN.patternloop_count == -1) {
	CHAN.patternloop_count = arg1;
    }
    if (CHAN.patternloop_count > 0) {
	player->pos = CHAN.patternloop_pos-1;
	CHAN.patternloop_count--;
    }
}

ARM_CommandType command_mod_do_pattern_loop_callbacks = { command_mod_do_pattern_loop_init, NULL, NULL, NULL, 0 };




/*
 * SET CHANNEL PANNING
 */

void command_mod_set_channel_pan_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "channel pan";
    CHAN.panning = arg1;
}

ARM_CommandType command_mod_set_channel_pan_callbacks = { command_mod_set_channel_pan_init, NULL, NULL, NULL, 0 };




/*
 * RETRIGGER
 */

void command_mod_retrigger_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "retrigger";
    CHAN.retrig_state.ticks_between = arg1;
    CHAN.retrig_state.volume_adjust = 0;
    CHAN.retrig_state.counter = 0;
}

void command_mod_retrigger_tick(struct ARM_Tracker* player, int c)
{
    if (CHAN.retrig_state.counter > CHAN.retrig_state.ticks_between) {
	CHAN.retrig_state.counter = 0;
	player->mixer->SetVoicePos(player->mixer, CHAN.voice, 0);
	switch (CHAN.retrig_state.volume_adjust) {
	case 0: break;
	case 1: CHAN.volume--; break;
	case 2: CHAN.volume -= 2; break;
	case 3: CHAN.volume -= 4; break;
	case 4: CHAN.volume -= 8; break;
	case 5: CHAN.volume -= 16; break;
	case 6: CHAN.volume *= 2; CHAN.volume /= 3; break;
	case 7: CHAN.volume /= 2; break;
	case 9: CHAN.volume++; break;
	case 10: CHAN.volume += 2; break;
	case 11: CHAN.volume += 4; break;
	case 12: CHAN.volume += 8; break;
	case 13: CHAN.volume += 16; break;
	case 14: CHAN.volume *= 3; CHAN.volume /= 2; break;
	case 15: CHAN.volume *= 2; break;
	};
    } else {
	CHAN.retrig_state.counter++;
    }
}

void command_mod_retrigger_cleanup(struct ARM_Tracker* player, int c)
{
}

ARM_CommandType command_mod_retrigger_callbacks = { command_mod_retrigger_init, command_mod_retrigger_tick, NULL, command_mod_retrigger_cleanup, 0 };




/*
 * FINE VOLUME SLIDE
 */

void command_mod_fine_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = (arg1 < 0) ? "fine volume down" : "fine volume up";
    CHAN.volume += (float)arg1;
    CLAMP(CHAN.volume, 0.0, 64.0);
}

ARM_CommandType command_mod_fine_volslide_callbacks = { command_mod_fine_volslide_init, NULL, NULL, NULL, 0 };




/*
 * CUT NOTE
 */

void command_mod_cut_channel_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "cut channel";
    CHAN.cut_channel = arg1;
}

void command_mod_cut_channel_tick(struct ARM_Tracker* player, int c)
{
    if (player->tick != 0) {
	if (player->tick >= CHAN.cut_channel) {
	    CHAN.volume = 0;
	}
    }
}

ARM_CommandType command_mod_cut_channel_callbacks = { command_mod_cut_channel_init, command_mod_cut_channel_tick, NULL, NULL, 0 };



/*
 * DELAY TRIGGER
 */

void command_mod_delay_trigger_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "delay trigger";
//    ARM_ResetChannel(&CHAN);
    CHAN.delay_trigger = arg1;
}

void command_mod_delay_trigger_tick(struct ARM_Tracker* player, int c)
{
    if (player->tick != 0) {
	if (CHAN.delay_trigger != 0 && player->tick >= CHAN.delay_trigger - 1) {
	    ARM_TriggerChannel(player, c,
			       CHAN.sample,
			       CHAN.period,
			       CHAN.volume);
	}
    }
}

ARM_CommandType command_mod_delay_trigger_callbacks = { command_mod_delay_trigger_init, command_mod_delay_trigger_tick, NULL, NULL, 1 };




/*
 * DELAY PATTERN
 */

void command_mod_delay_pattern_init(struct ARM_Tracker* player, int c, int arg1, int arg2)
{
    CHAN.command_name = "delay pattern";
    player->patterndelay = arg1;
}

ARM_CommandType command_mod_delay_pattern_callbacks = { command_mod_delay_pattern_init, NULL, NULL, NULL, 0 };

