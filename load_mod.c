#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "tracker.h"
#include "commands.h"
#include "commands_mod.h"
#include "osutil.h"

void ARM_FreeSampleData(ARM_Sample* sam);
void ARM_FreePatternData(ARM_Pattern* pat);

int ARM_ConvertFinetuneToC4SPD(int finetune)
{
    const int ftab[] = {
	7895, 7941, 7985, 8046,
	8107, 8169, 8232, 8280,
	8363, 8413, 8463, 8529,
	8581, 8651, 8723, 8757
    };

    if (finetune < -8 || finetune > 7)
	finetune = 0;

    return ftab[finetune + 8];
}

int ARM_MOD_InstallCommand(ARM_Note* note, uint8_t code, uint8_t argx, uint8_t argy)
{
    uint8_t argz;

    memset(&note->cmd, 0, sizeof(ARM_Command));
    note->cmd.cmd = 0;

    argz = ((argx & 0x0F) << 4) + (argy & 0x0F);

    switch (code) {
    case 0:
	if (argx == 0 && argy == 0) {
	    /* Null effect. */
	    break;
	}
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_arpeggio_callbacks);
	note->cmd.arg1 = (int)argx;
	note->cmd.arg2 = (int)argy;
	break;
    case 1:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_period_slide_callbacks);
	note->cmd.arg1 = -((int)argx*16+argy)*4;
	break;
    case 2:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_period_slide_callbacks);
	note->cmd.arg1 = ((int)argx*16+argy)*4;
	break;
    case 3:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_slide_to_note_callbacks);
	note->cmd.arg1 = ((int)argx*16+argy)*4;
	note->cmd.arg2 = note->note;
	note->trigger = 0;
	break;
    case 4:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_vibrato_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = (int)argy * 4;
	break;
    case 5:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_period_and_volslide_callbacks);
	if (argx != 0)
	    note->cmd.arg1 = argx;
	else
	    note->cmd.arg1 = -(int)argy;
	note->trigger = 0;
	break;
    case 6:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_volslide_and_vibrato_callbacks);
	if (argx != 0)
	    note->cmd.arg1 = argx;
	else
	    note->cmd.arg1 = -(int)argy;
	break;
    case 7:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_tremolo_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = (int)argy*4;
	break;
    case 8:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_null_callbacks);
	break;
    case 9:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_offset_callbacks);
	note->cmd.arg1 = (uint32_t)argx * 4096 + argy * 256;
	break;
    case 10:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_volslide_callbacks);
	if (argx != 0)
	    note->cmd.arg1 = argx;
	else
	    note->cmd.arg1 = -(int)argy;
	break;
    case 11:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_goto_callbacks);
	note->cmd.arg1 = argz;
	note->cmd.arg2 = -1;
	break;
    case 12:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_volume_callbacks);
	note->cmd.arg1 = argz;
	if (note->cmd.arg1 > 64) note->cmd.arg1 = 64;
	break;
    case 13:
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_goto_callbacks);
	note->cmd.arg1 = -1;
	note->cmd.arg2 = argx * 10 + argy;
	break;
    case 14:
	switch (argx) {
	case 0:
	    /* Maybe at some point we can implement filter control of some sort. */
	    break;
	case 1:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_fine_period_slide_callbacks);
	    note->cmd.arg1 = -(int)argy*4;
	    break;
	case 2:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_fine_period_slide_callbacks);
	    note->cmd.arg1 = (int)argy*4;
	    break;
	case 5:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_c4spd_callbacks);
	    if (argy & 8) argy |= 0xF0;
	    note->cmd.arg1 = ARM_ConvertFinetuneToC4SPD(argy);
	    break;
	case 6:
	    if (argy == 0) {
		note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_pattern_loop_callbacks);
	    } else {
		note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_do_pattern_loop_callbacks);
		note->cmd.arg1 = argy;
	    }
	    break;
	case 8:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_channel_pan_callbacks);
	    note->cmd.arg1 = ((int)argy * 255 / 15);
	    break;
	case 9:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_retrigger_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	case 10:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_fine_volslide_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	case 11:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_fine_volslide_callbacks);
	    note->cmd.arg1 = -(int)argy;
	    break;
	case 12:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_cut_channel_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	case 13:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_delay_trigger_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	case 14:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_delay_pattern_callbacks);
	    note->cmd.arg1 = argy;
	    break;			    
	default:
	    printf("FIXME: MOD effect converter: unhandled effect 14.%i (%i)\n", argx, argy);
	}
	break;
    case 15:
	if (argz == 0)
	    argz = 1;
	if (argz <= 32) {
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_speed_callbacks);
	    note->cmd.arg1 = argz;
	} else {
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_tempo_callbacks);
	    note->cmd.arg1 = argz;
	}
	break;
    default:
	printf("FIXME: MOD effect converter: unhandled effect %i (%i,%i)\n", code, argx, argy);
    }

    return 0;
}

int ARM_LoadModule_MOD(ARM_Module* mod, const char *filename)
{
    FILE *f;
    char title[23];
    char sig[5];
    uint16_t tmp16;
    uint8_t tmpu8;
    int max_pattern;
    int i;
    
    memset(mod, 0, sizeof (ARM_Module));

    f = fopen(filename, "rb");
    if (f == NULL)
	return -1;
    
    /* Read title. */
    if (fread(title, 20, 1, f) != 1) goto failure;
    title[20] = '\0';
    mod->title = strdup(title); if (mod->title == NULL) goto failure;

    /* Fill in default volume. */
    mod->global_volume = 64;
    mod->master_volume = 64;

    /* Read 31 sample headers. */
    mod->num_samples = 31;
    mod->samples = (ARM_Sample*)malloc(31 * sizeof (ARM_Sample));
    if (mod->samples == NULL) goto failure;
    memset(mod->samples, 0, 31 * sizeof (ARM_Sample));
    for (i = 0; i < 31; i++) {
	int8_t finetune;

	if (fread(title, 22, 1, f) != 1) goto failure;
	title[22] = '\0';
	mod->samples[i].name = strdup(title);
	if (mod->samples[i].name == NULL) goto failure;

	fread(&tmp16, 2, 1, f);
	mod->samples[i].length = SWAP_BE_16(tmp16) * 2;
	fread(&finetune, 1, 1, f);
	finetune &= 0x0F;
	if (finetune & 8) finetune |= 0xF0;
	mod->samples[i].c4spd = ARM_ConvertFinetuneToC4SPD(finetune);
	fread(&tmpu8, 1, 1, f);
	mod->samples[i].volume = (float)tmpu8;
	fread(&tmp16, 2, 1, f);
	mod->samples[i].repeat_ofs = SWAP_BE_16(tmp16) * 2;
	if (fread(&tmp16, 2, 1, f) != 1) goto failure;
	mod->samples[i].repeat_len = SWAP_BE_16(tmp16) * 2;
	if (mod->samples[i].repeat_len > 2) mod->samples[i].repeat_enabled = 1;
	else mod->samples[i].repeat_enabled = 0;
    }

    /* Read order info. */
    fread(&tmpu8, 1, 1, f);
    mod->num_order = tmpu8;
    mod->order = (int *)malloc(128 * sizeof (int));
    if (mod->order == NULL) goto failure;
    memset(mod->order, 0, 128 * sizeof (int));
    fread(&tmpu8, 1, 1, f); /* junk */
    max_pattern = 0;
    for (i = 0; i < 128; i++) {
	fread(&tmpu8, 1, 1, f);
	mod->order[i] = (int)tmpu8;
	if (mod->order[i] > max_pattern)
	    max_pattern = mod->order[i];
    }

    /* Read signature. */
    if (fread(sig, 4, 1, f) != 1) goto failure;
    sig[4] = '\0';
    
    mod->num_channels = 0;
    if (!strcmp(sig, "M.K.")) {
	mod->num_channels = 4;
    } else if (!strcmp(sig, "6CHN") || !strcmp(sig, "FLT6")) {
	mod->num_channels = 6;
    } else if (!strcmp(sig, "8CHN")) {
	/* We don't support the FLT8 format (StarTrekker). It achieves 8 channels
	   by pairing patterns side by side. I've never actually run into one of
	   these files, though... */
	mod->num_channels = 8;
    } else {
	printf("Unknown MOD format. Signature was '%s'.", sig);
	goto failure;
    }

    mod->default_pan = (int *)malloc(mod->num_channels * sizeof (int));
    if (mod->default_pan == 0)
	goto failure;
    for (i = 0; i < mod->num_channels; i++) {
	if (i % 4 == 0) mod->default_pan[i] = 0;
	if (i % 4 == 1) mod->default_pan[i] = 255;
	if (i % 4 == 2) mod->default_pan[i] = 255;
	if (i % 4 == 3) mod->default_pan[i] = 0;
    }

    /* Read patterns. */
    mod->num_patterns = max_pattern + 1;
    mod->patterns = (ARM_Pattern*)malloc(mod->num_patterns * sizeof (ARM_Pattern));
    if (mod->patterns == NULL) goto failure;
    memset(mod->patterns, 0, mod->num_patterns * sizeof (ARM_Pattern));
    for (i = 0; i < mod->num_patterns; i++) {
	int d, c;

	if (ARM_AllocPatternData(&mod->patterns[i], 64, mod->num_channels) != 0) goto failure;

	for (d = 0; d < 64; d++) {
	    for (c = 0; c < mod->num_channels; c++) {
		uint8_t raw_note[4];
		uint32_t raw_cmd;
		uint8_t argx, argy;
		ARM_Note* note;
		int command;
		int sample_num;
		unsigned int period;

		note = ARM_GetPatternNote(&mod->patterns[i], d, c);
		assert(note != NULL);

		if (fread(raw_note, 4, 1, f) != 1) goto failure;
		sample_num = (raw_note[0] & 0xF0) + ((raw_note[2] >> 4) & 0x0F);
		CLAMP(sample_num, 0, mod->num_samples);
		note->sample = sample_num - 1;
		period = ((raw_note[1] + ((raw_note[0] & 0x0F) * 256)) * 4);
		if (period != 0)
		    note->trigger = 1;
		else
		    note->trigger = 0;
		if (period == 0)
		    note->note = 0;
		else
		    note->note = ARM_FindNoteForLogPeriod(period / 1712.0);
		raw_cmd = raw_note[3] + (raw_note[2] & 0x0F) * 256;

		/* MOD's don't have a volume column. */
		note->volume = -1.0;

		/* Install the command handler. */
		command = (raw_cmd & 0x0F00) >> 8;
		argx = (raw_cmd & 0x00F0) >> 4;
		argy = (raw_cmd & 0x000F);
	        ARM_MOD_InstallCommand(note, command, argx, argy);

	    }
	}
    }

    /* Load samples. */
    for (i = 0; i < 31; i++) {
	int8_t *sam;
	unsigned int s;

	/* MOD samples are signed 8-bit. */
	sam = (char *)mallocsafe(mod->samples[i].length);
	fread(sam, mod->samples[i].length, 1, f);
	mod->samples[i].data = (float *)mallocsafe(mod->samples[i].length * sizeof (float));
	
	/* Convert to normalized float. */
	for (s = 0; s < mod->samples[i].length; s++) {
	    mod->samples[i].data[s] = (float)sam[s] / 128.0;
	}
	
	free(sam);
    }

    mod->period_mode = ARM_PERIOD_LOG;

    fclose(f);

    return 0;

 failure:
    if (f != NULL) fclose(f);
    ARM_FreeModuleData(mod);
    return -1;
}
