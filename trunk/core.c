#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tracker.h"
#include "commands_mod.h"

#define CHAN player->channels[c]



void ARM_ResetChannel(ARM_Tracker* player, int c)
{
    /* Stop playback of this channel. */
    player->mixer->StopVoice(player->mixer, CHAN.voice);

    /* Cancel delay trigger and note cut effects. */
    CHAN.delay_trigger = -1;
    CHAN.cut_channel = -1;
    
    /* Clear the current command. */
    memset(&CHAN.command, 0, sizeof (ARM_Command));
}

/*
 * Immediately jumps to the given order and row, and tick 0.
 */
void ARM_StartPattern(ARM_Tracker* player, int order, int pos)
{
    if (order >= player->mod->num_order)
	order = player->mod->num_order - 1;
    player->pattern = player->mod->order[order];
    
    if (player->pattern == 255) {
	/* End of module. However, patternbreaking past
	   the end should wrap to the beginning. */
	if (player->patternbreak >= 0) {
	    ARM_StartPattern(player, 0, pos);
	    return;
	}
	player->done = 1;
	player->pattern = player->mod->order[0];
    }
    player->order = order;
    player->pos = pos;
    player->tick = 0;
}

/* Gives the period that is the given number of semitones above or below the given note.
   Calculated with the FPU, not a table. */
float ARM_CalcSemitones(float period, float semitones)
{
    return period * pow(2.0, ((float)semitones/-12.0));
}

const char* ARM_GetNameForNote(int note)
{
    const char* names[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    unsigned int semitone = (unsigned int)note % 12;
    return (semitone < 12 ? names[semitone] : "??");
}

float ARM_GetLogPeriodForNote(int note)
{
    float semitones = note - ARM_MIDDLE_C_NOTE;
    return pow(2.0, semitones / -12.0);
}

float ARM_GetPeriodForNote(ARM_Tracker* player, int note)
{
    switch (player->mod->period_mode) {   
    case ARM_PERIOD_LOG:
	return ARM_GetLogPeriodForNote(note);
    default: return 0.0;
    }
}

int ARM_FindNoteForLogPeriod(float period)
{
    return (int)floor(log(period) / log(2.0) * -12.0 + (float)ARM_MIDDLE_C_NOTE + 0.5);
}

int ARM_GetOctaveForNote(int note)
{
    return note/12;
}

void ARM_TriggerChannel(ARM_Tracker* player, int c, int sample, int note, float volume)
{
    ARM_Sample *sample_ptr;

    if (sample == -1 || sample >= player->mod->num_samples)
	return;

    sample_ptr = &player->mod->samples[sample];

    CHAN.period = ARM_GetPeriodForNote(player, note);
    CHAN.sample = sample;

    if (volume >= 0.0) {
	CHAN.volume = volume;
    } else {
	CHAN.volume = sample_ptr->volume;
    }
    CHAN.c4spd = sample_ptr->c4spd;

    CHAN.cut_channel = -1;
    CHAN.delay_trigger = -1;
    if (CHAN.vibrato_state.retrig)
	CHAN.vibrato_state.pos = 0;
    CHAN.vibrato_state.enabled = 0;
    if (CHAN.tremolo_state.retrig)
	CHAN.tremolo_state.pos = 0;
    CHAN.tremolo_state.enabled = 0;

    /* Program voice. */
    player->mixer->SetVoicePatch(player->mixer,
				 CHAN.voice,
				 sample_ptr->patch);
    player->mixer->SetVoicePos(player->mixer,
			       CHAN.voice,
			       0);

    /* Program looping? */
    if (sample_ptr->repeat_enabled) {
	player->mixer->SetVoiceLoopPos(player->mixer,
				       CHAN.voice,
				       sample_ptr->repeat_ofs,
				       sample_ptr->repeat_len);
	player->mixer->SetVoiceLoopMode(player->mixer,
					CHAN.voice,
					1);
    } else {
	player->mixer->SetVoiceLoopMode(player->mixer,
					CHAN.voice,
					0);
    }

    /* Trigger the channel. */
    player->mixer->TriggerVoice(player->mixer, CHAN.voice);
}

void ARM_AdvancePosition(ARM_Tracker* player)
{
    int c;

    player->tick++;
    if (player->tick >= player->speed) {
	player->tick = 0;
	if (player->patternbreak >= 0 || player->goto_pattern >= 0) {
	    int row = player->pos, order = player->order;
	    if (player->goto_pattern >= 0) {
		order = player->goto_pattern;
		if (order >= player->mod->num_order)
		    order = 0;
		row = 0;
	    }
	    if (player->patternbreak >= 0) {
		row = player->patternbreak;
		if (player->goto_pattern < 0)
		    order++;
	    }
	    ARM_StartPattern(player, order, row);
	    player->patternbreak = -1;
	    player->goto_pattern = -1;
	} else if (player->patterndelay <= 0) {
	    player->pos++;
	    if (player->pos >= 64) {
		player->order++;
		while (player->order < player->mod->num_order &&
		       player->mod->order[player->order] == 254)
		    player->order++;
		if (player->order < player->mod->num_order &&
		    player->mod->order[player->order] != 255)
		    ARM_StartPattern(player, player->order, 0);
		else {
		    player->done = 1;
		    ARM_StartPattern(player, 0, 0);
		}
		for (c = 0; c < player->num_channels; c++) {
		    CHAN.patternloop_pos = 0;
		    CHAN.patternloop_count = -1;
		}
	    }
	}
    }
}

static void ProcessNewRow(ARM_Tracker* player)
{
    int c;
    int pattern;
    int pos;
    int sample;    

    pattern = player->pattern;
    pos = player->pos;

    if (player->patterndelay > 0) {
	player->patterndelay--;
	return;
    }

    printf("%3i: ", player->pos);

    for (c = 0; c < player->num_channels && c < 16; c++) {
	ARM_Note* note;
	ARM_CommandType* callbacks;

	CHAN.command_name = "";

	/* Clean up after the previous row's command, if any. */
	callbacks = ARM_GetCallbacksForNum(CHAN.command.cmd);
	if (callbacks->cleanup_proc != NULL)
	    callbacks->cleanup_proc(player, c);
	
	note = ARM_GetPatternNote(&player->mod->patterns[pattern], pos, c);

	if (note == NULL)
	    continue;

	callbacks = ARM_GetCallbacksForNum(note->cmd.cmd);

	if (!(callbacks->no_reset_sample)
	    && note->sample != -1)
	    sample = note->sample;
	else
	    sample = CHAN.sample;

	if (note->note == 255) {
	    /* Handle keyoff. */
	    ARM_ResetChannel(player, c);
        } else if (note->trigger) {
	    /* Trigger this note. */
	    ARM_TriggerChannel(player, c, sample, note->note, note->volume);
	} else {
	    /* If this isn't a trigger but there is a sample, reset volume. */
	    if (note->sample != -1 && CHAN.sample != -1)
		CHAN.volume = player->mod->samples[CHAN.sample].volume;
	}

	/* If an explicit volume was given, use that. */
	if (note->volume >= 0.0) {
	    CHAN.volume = note->volume;
	}

	/* Kill current commands */
	CHAN.vibrato_state.enabled = 0;
	CHAN.tremolo_state.enabled = 0;

	/* Install command. */
	CHAN.command = note->cmd;

	/* Call the command initialization proc. */
	if (callbacks->init_proc != NULL)
	    callbacks->init_proc(player, c, note->cmd.arg1, note->cmd.arg2);

	if (note->note == 255 || note->note == 0) {
	    printf("    |");
	} else {
	    printf("%3s%i|", ARM_GetNameForNote(note->note), ARM_GetOctaveForNote(note->note));
	}
    }

    printf("\n");

}

void ARM_RenderOneTick(ARM_Tracker* player, float mix_divisor)
{
    int i, c;
    int ticks_per_second;

    if (player->tick == 0) {
	ProcessNewRow(player);
    }

    /* Figure out our playback speed.
       We have (24 * bpm / 60) ticks per second. */
    ticks_per_second = 24 * player->bpm / 60;

    player->frames_per_tick = (int)((float)player->mixrate / ticks_per_second);
    
    for (c = 0; c < player->num_channels; c++) {
	ARM_Sample* sample;
	float tremolo_volume_shift = ARM_MOD_CalcTremolo(player, c);
	float v, freq, period, c4spd;
	ARM_CommandType* callbacks;

	/* Apply the current effect. */
	callbacks = ARM_GetCallbacksForNum(CHAN.command.cmd);
	if (callbacks->pre_tick_proc != NULL)
	    callbacks->pre_tick_proc(player, c);

	/* Only update volume, freq, etc if there is a sample. */
	if (CHAN.sample != -1) {
	    v = (CHAN.volume + tremolo_volume_shift) / 64.0;
/*	    v = v * player->mod->master_volume / 64.0;*/
	    v = v * player->mod->global_volume / 64.0;
	    CLAMP(v, 0.0, 1.0);
	    
	    /* Calculate frequency. */
	    period = CHAN.period;

	    /* Prevent division by zero. */
	    LOWER_CLAMP(period, 0.0001);
	    if (CHAN.vibrato_state.enabled)
		period += CHAN.vibrato_state.period_shift;
	    c4spd = CHAN.c4spd;
	    freq = c4spd / period;
	    
	    /* Update the channel for this tick. */
	    player->mixer->SetVoiceRate(player->mixer,
					CHAN.voice,
					freq);
	    player->mixer->SetVoiceVolume(player->mixer,
					  CHAN.voice,
					  v);
	    player->mixer->SetVoiceBalance(player->mixer,
					   CHAN.voice,
					   (CHAN.panning - 128.0) / 128.0);
	}	    

	/* Apply the current effect. */
	if (callbacks->post_tick_proc != NULL)
	    callbacks->post_tick_proc(player, c);	
	
    }

    /* Render. */
    player->mixer->Render(player->mixer, player->frames_per_tick, mix_divisor);

}
    
void ARM_InitTracker(ARM_Tracker* player, ARM_Module* mod, Mixer* mixer, int mixrate)
{
    int i;

    player->mixer = mixer;
    player->done = 0;
    player->mixrate = mixrate;
    player->mod = mod;
    player->num_channels = mod->num_channels;
    player->channels = (ARM_Channel*)mallocsafe(mod->num_channels * sizeof (ARM_Channel));

    /* Initialize each channel. */
    for (i = 0; i < mod->num_channels; i++) {
	memset(&player->channels[i], 0, sizeof (ARM_Channel));
	player->channels[i].sample = -1;
	player->channels[i].panning = mod->default_pan[i];
	player->channels[i].patternloop_count = -1;
	player->channels[i].voice = player->mixer->AllocVoice(player->mixer, 0);
	if (player->channels[i].voice < 0) {
	    fatal("Unable to allocate a voice.\n");
	}
	ARM_ResetChannel(player, i);
    }

    /* Upload patches. */
    for (i = 0; i < mod->num_samples; i++) {
	mod->samples[i].patch = player->mixer->LoadPatch(player->mixer,
							 mod->samples[i].data,
							 mod->samples[i].length);
    }

    player->order = 0;
    player->speed = (mod->initial_speed != 0 ? mod->initial_speed : 6);
    player->bpm = (mod->initial_bpm != 0 ? mod->initial_bpm : 125);
    ARM_StartPattern(player, 0, 0);
    player->goto_pattern = -1;
    player->patternbreak = -1;
    player->patterndelay = 0;
    player->pos = 0;
    player->tick = 0;
}

void ARM_FreeTrackerData(ARM_Tracker* player)
{
    int i;

    /* Free voices. */
    for (i = 0; i < player->num_channels; i++) {
	if (player->channels[i].voice >= 0)
	    player->mixer->FreeVoice(player->mixer, player->channels[i].voice);
    }

    /* Unload patches from the mixer. */
    for (i = 0; i < player->mod->num_samples; i++) {
	if (player->mod->samples[i].patch >= 0)
	    player->mixer->UnloadPatch(player->mixer, player->mod->samples[i].patch);
    }
    
    /* Free channel data. */
    free(player->channels);
}
