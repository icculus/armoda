#include <stdio.h>
#include <stdlib.h>
#include "tracker.h"
#include "load_s3m.h"
#include "commands.h"
#include "commands_mod.h"
#include "commands_s3m.h"

/* S3M period table. Borrowed from FireMod. */
static uint32_t note_to_period[134] = {
  27392,25856,24384,23040,21696,20480,19328,18240,17216,16256,15360,14496,
  13696,12928,12192,11520,10848,10240,9664, 9120, 8608, 8128, 7680, 7248,
  6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4064, 3840, 3624,
  3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1812,
  1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960,  906,
  856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
  428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,
  214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,
  107,  101,  95,   90,   85,   80,   75,   71,   67,   63,   60,   56,
  53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28,
  26,   25,   23,   22,   21,   20,   18,   17,   16,   15,   15,   14,
  0,    0
};



static int InstallCommand(ARM_Note* note, uint8_t code, uint8_t argx, uint8_t argy)
{
    uint8_t argz;

    memset(&note->cmd, 0, sizeof(ARM_Command));
    note->cmd.cmd = 0;

    argz = ((argx & 0x0F) << 4) + (argy & 0x0F);

    switch (code - 1 + 'A') {
    case 'A': 
	/* Identical to MOD set speed, but allows a wider range. */
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_speed_callbacks);
	note->cmd.arg1 = argz;
	if (note->cmd.arg1 == 0)
	    note->cmd.arg1 = 1;
	break;

    case 'B':
	/* Identical to MOD pattern jump. */
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_goto_callbacks);
	note->cmd.arg1 = argz;
	note->cmd.arg2 = -1;
	break;

    case 'C':
	/* Identical to MOD pattern break. */
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_goto_callbacks);
	note->cmd.arg1 = -1;
	note->cmd.arg2 = argx * 10 + argy;
	break;

    case 'D':
	note->cmd.arg1 = argx;
	note->cmd.arg2 = argy;
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_volslide_callbacks);
	break;

    case 'E':
	note->cmd.arg1 = argx;
	note->cmd.arg2 = argy;
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_period_slide_down_callbacks);
	break;

    case 'F':
	note->cmd.arg1 = argx;
	note->cmd.arg2 = argy;
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_period_slide_up_callbacks);
	break;

    case 'G':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_slide_to_note_callbacks);
	note->cmd.arg1 = (int)argz*4;
	note->cmd.arg2 = note->period;
	note->trigger = 0;
	break;

    case 'H':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_vibrato_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = (int)argy * 4;
	break;

    case 'I':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_tremor_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = argy;
	break;

    case 'J':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_arpeggio_callbacks);
	note->cmd.arg1 = (int)argx;
	note->cmd.arg2 = (int)argy;
	break;

    case 'K':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_volslide_and_vibrato_callbacks);
	if (argx != 0)
	    note->cmd.arg1 = argx;
	else
	    note->cmd.arg1 = -(int)argy;
	break;

    case 'L':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_period_and_volslide_callbacks);
	if (argx != 0)
	    note->cmd.arg1 = argx;
	else
	    note->cmd.arg1 = -(int)argy;
	note->trigger = 0;
	break;

    case 'O':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_offset_callbacks);
	note->cmd.arg1 = (uint32_t)argz << 8;
	break;

    case 'Q':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_retrig_and_volslide_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = argy;
	break;

    case 'R':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_tremolo_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = (int)argy * 4;
	break;

    case 'S':
	switch (argx) {
	case 0:
	    /* amiga filter set */
	    break;
	case 1:
	    /* enable glissando on porta to note */
	    break;
	case 2:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_c4spd_callbacks);
	    if (argy & 8) argy |= 0xF0;
	    note->cmd.arg1 = ARM_ConvertFinetuneToC4SPD(argy);
	    break;
	case 3:
	    /* set vibrato waveform */
	    break;
	case 4:
	    /* set tremolo waveform */
	    break;
	case 8:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_channel_pan_callbacks);
	    note->cmd.arg1 = (int)argy * 255 / 15;
	    break;
	case 0xA:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_channel_pan_callbacks);
	    note->cmd.arg1 = (argy > 7 ? (int)argy - 8 : (int)argy + 8);
	    note->cmd.arg1 = note->cmd.arg1 * 255 / 15;
	    break;
	case 0xB:
	    if (argy == 0) {
		note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_pattern_loop_callbacks);
		note->cmd.arg1 = 0;
	    } else {
		note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_do_pattern_loop_callbacks);
		note->cmd.arg1 = argy;
	    }
	    break;
	case 0xC:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_cut_channel_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	case 0xD:
	    printf("DEBUG: delay trigger disabled\n");
/*	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_delay_trigger_callbacks); */
	    note->cmd.arg1 = argy;
	    break;
	case 0xE:
	    note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_delay_pattern_callbacks);
	    note->cmd.arg1 = argy;
	    break;
	default:
	    printf("DEBUG: unrecognized S command %X\n", argx);
	}
	break;

    case 'T':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_mod_set_tempo_callbacks);
	note->cmd.arg1 = argz;
	break;

    case 'U':
	note->cmd.cmd = ARM_GetNumForCallbacks(&command_s3m_vibrato_callbacks);
	note->cmd.arg1 = argx;
	note->cmd.arg2 = (int)argy;
	break;

    default:
	printf("DEBUG: unrecognized S3M effect %c (param %2X)\n",
	       code - 1 + 'A', argz);
    }
    
    return 0;
}


int ARM_LoadModule_S3M(ARM_Module* mod, const char *filename)
{
    FILE *f;
    uint16_t header_flags;
    uint32_t tmpu32;
    uint16_t tmpu16;
    uint8_t tmpu8;
    char title[28];
    char scrm[5];
    int signed_samples = 0;
    int has_default_pan = 0;
    int i;
    int mono;

    memset(mod, 0, sizeof (ARM_Module));
    
    f = fopen(filename, "r");
    if (f == NULL) goto failure;

    /* Read the title */
    fread(&title, 28, 1, f);
    title[27] = '\0';
    mod->title = strdup(title); if (mod->title == NULL) goto failure;

    /* Check the signature */
    fread(&tmpu8, 1, 1, f);
    if (tmpu8 != 0x1A) goto failure;

    /* Check the module type */
    fread(&tmpu8, 1, 1, f);
    if (tmpu8 != 16) goto failure;

    fread(&tmpu16, 2, 1, f);

    /* Read various counts */
    fread(&tmpu16, 2, 1, f);
    mod->num_order = SWAP_LE_16(tmpu16);
    mod->order = (int *)calloc(mod->num_order, sizeof (int));
    if (mod->order == NULL) goto failure;
    fread(&tmpu16, 2, 1, f);
    mod->num_samples = SWAP_LE_16(tmpu16);
    mod->samples = (ARM_Sample *)calloc(mod->num_samples, sizeof (ARM_Sample));
    if (mod->samples == NULL) goto failure;
    fread(&tmpu16, 2, 1, f);
    mod->num_patterns = SWAP_LE_16(tmpu16);
    mod->patterns = (ARM_Pattern *)calloc(mod->num_patterns, sizeof (ARM_Pattern));
    if (mod->patterns == NULL) goto failure;
    
    /* Read flags */
    fread(&header_flags, 2, 1, f);
    header_flags = SWAP_LE_16(header_flags);
    if (header_flags & 16) mod->flags |= MODULE_FLAG_ENFORCE_AMIGA_LIMITS;
    if (header_flags & 64)  mod->flags |= MODULE_FLAG_VOLSLIDE_ON_FIRST_TICK;
    if (header_flags & 8) mod->flags |= MODULE_FLAG_KILL_SILENT_LOOPS;
    
    fread(&tmpu16, 2, 1, f);
    tmpu16 = SWAP_LE_16(tmpu16);
    if (tmpu16 == 0x1300) mod->flags |= MODULE_FLAG_VOLSLIDE_ON_FIRST_TICK;
    
    /* Read file format version */
    fread(&tmpu16, 2, 1, f);
    tmpu16 = SWAP_LE_16(tmpu16);
    switch (tmpu16) {
    case 1: signed_samples = 1; break;
    case 2: signed_samples = 0; break;
    default: goto failure;
    }

    /* Read the signature */
    fread(scrm, 4, 1, f);
    scrm[4] = '\0';
    if (strcmp(scrm, "SCRM")) goto failure;

    /* Read various info */
    fread(&tmpu8, 1, 1, f);
    mod->global_volume = tmpu8;

    fread(&tmpu8, 1, 1, f);
    mod->initial_speed = tmpu8;
    fread(&tmpu8, 1, 1, f);
    mod->initial_bpm = tmpu8;

    fread(&tmpu8, 1, 1, f);
    mod->master_volume = tmpu8 & 127;
    if (tmpu8 & 128) {
	mono = 1;
    } else mono = 0;

    fread(&tmpu8, 1, 1, f);
    fread(&tmpu8, 1, 1, f);
    if (tmpu8 == 0xFC) {
	has_default_pan = 1;
    } else {
	has_default_pan = 0;
    }

    /* Jump to channel settings */
    fseek(f, 10, SEEK_CUR);
    mod->num_channels = 32;
    mod->default_pan = (int *)calloc(mod->num_channels, sizeof (int));
    if (mod->default_pan == NULL) goto failure;
    for (i = 0; i < 32; i++) {
	fread(&tmpu8, 1, 1, f);
	if (!has_default_pan)
	    mod->default_pan[i] = tmpu8 * 16;
	else
	    mod->default_pan[i] = 127;
    }

    /* Read the order list */
    for (i = 0; i < mod->num_order; i++) {
	fread(&tmpu8, 1, 1, f);
	mod->order[i] = tmpu8;
    }

    /* Read in samples. */
    for (i = 0; i < mod->num_samples; i++) {
	long int filepos;
	int samplepos;
	int is_stereo = 0;
	int is_16bit = 0;
	uint32_t memseg;
	void *buf;

	fread(&tmpu16, 2, 1, f);
	tmpu16 = SWAP_LE_16(tmpu16);

	/* Save the current position and jump to the sample data. */
	filepos = ftell(f);
	samplepos = tmpu16 * 16;
	fseek(f, samplepos, SEEK_SET);

	/* Read the data. */
	fread(&tmpu8, 1, 1, f);
	if (tmpu8 == 0) {
	    fseek(f, filepos, SEEK_SET);
	    continue;
	}
	if (tmpu8 != 1) {
	    printf("warning: sample %i is not a PCM sample (%i).\n", i, tmpu8);
	    goto failure;
	}

	mod->samples[i].filename = (char *)malloc(13);
	if (mod->samples[i].filename == NULL) goto failure;
	fread(mod->samples[i].filename, 12, 1, f);
	mod->samples[i].filename[12] = '\0';
	
	memseg = 0;
	fread(&tmpu8, 1, 1, f);
	memseg += (int)tmpu8 << 16;
	fread(&tmpu8, 1, 1, f);
	memseg += (int)tmpu8;
	fread(&tmpu8, 1, 1, f);
	memseg += (int)tmpu8 << 8;
	memseg *= 16;

	fread(&tmpu32, 4, 1, f);
	mod->samples[i].length = SWAP_LE_32(tmpu32);
	
	fread(&tmpu32, 4, 1, f);
	mod->samples[i].repeat_ofs = SWAP_LE_32(tmpu32);
	fread(&tmpu32, 4, 1, f);
	mod->samples[i].repeat_len = SWAP_LE_32(tmpu32) - mod->samples[i].repeat_ofs;

	fread(&tmpu8, 1, 1, f);
	mod->samples[i].volume = tmpu8;
	
	fread(&tmpu8, 1, 1, f);
	fread(&tmpu8, 1, 1, f);
	if (tmpu8 != 0) {
	    goto failure;
	}

	fread(&tmpu8, 1, 1, f);
	if (tmpu8 & 1) mod->samples[i].repeat_enabled = 1;
	if (tmpu8 & 2) { is_stereo = 1; goto failure; } /* FIXME: not handled yet */
	if (tmpu8 & 4) is_16bit = 1;

	fread(&tmpu32, 4, 1, f);
	mod->samples[i].c4spd = SWAP_LE_32(tmpu32);
	fseek(f, 12, SEEK_CUR);

	mod->samples[i].name = (char *)malloc(28);
	if (mod->samples[i].name == NULL) goto failure;
	fread(mod->samples[i].name, 28, 1, f);
	mod->samples[i].name[27] = '\0';

	fseek(f, memseg, SEEK_SET);
		
	buf = (int8_t *)mallocsafe(mod->samples[i].length);
	fread(buf, mod->samples[i].length, 1, f);

	if (is_16bit)
	    mod->samples[i].length /= 2;

	mod->samples[i].data = (float *)mallocsafe(mod->samples[i].length * sizeof (float));

	if (is_16bit) {
	    unsigned int s;
	    if (signed_samples) {
		int16_t *sbuf = (int16_t *)buf;		
		for (s = 0; s < mod->samples[i].length; s++) {
		    mod->samples[i].data[s] = (float)SWAP_LE_16(sbuf[i]) / 32768.0;
		}
	    } else {
		uint16_t *ubuf = (uint16_t *)buf;
		for (s = 0; s < mod->samples[i].length; s++) {
		    mod->samples[i].data[s] = (((float)SWAP_LE_16(ubuf[i]) / 65535.0) - 0.5) * 2.0;
		}
	    }
	} else {
	    unsigned int s;
	    if (signed_samples) {
		int8_t *sbuf = (int8_t *)buf;
		for (s = 0; s < mod->samples[i].length; s++) {
		    mod->samples[i].data[s] = (float)sbuf[s] / 128.0;
		}
	    } else {
		uint8_t *ubuf = (uint8_t *)buf;
		for (s = 0; s < mod->samples[i].length; s++) {
		    mod->samples[i].data[s] = (((float)ubuf[s] / 255.0) - 0.5) * 2.0;
		}
	    }
	}

	free(buf);

	/* Go back to the parapointers. */
	fseek(f, filepos, SEEK_SET);
    }

    /* Read in the patterns. */
    for (i = 0; i < mod->num_patterns; i++) {
	int filepos;
	int patternpos;
	int row, chan;

	fread(&tmpu16, 2, 1, f);

	/* Jump to the pattern. */
	filepos = ftell(f);
	patternpos = SWAP_LE_16(tmpu16) * 16;
	fseek(f, patternpos, SEEK_SET);	
	
	/* Decode. */
	fread(&tmpu16, 2, 1, f);
	tmpu16 = SWAP_LE_16(tmpu16);

	if (ARM_AllocPatternData(&mod->patterns[i], 64, 32) != 0) goto failure;	

	row = 0;
	chan = 0;
	while (row < 64) {
	    uint8_t tmp2;

	    fread(&tmpu8, 1, 1, f);
	    if (tmpu8 > 0) {
		ARM_Note note;
		
		memset(&note, 0, sizeof (ARM_Note));
		note.sample = -1;
		note.cmd.cmd = 0;
				
		chan = tmpu8 & 31;
		if (tmpu8 & 32) {

		    fread(&tmp2, 1, 1, f);
		    if (tmp2 == 255) {
		    } else if (tmp2 == 254) {
			note.period = 0xFFFFFFFF;
		    } else {
			note.trigger = 1;
			note.period = note_to_period[(tmp2 >> 4) * 12 + (tmp2 & 0x0F)];
		    }
		    
		    fread(&tmp2, 1, 1, f);
		    UPPER_CLAMP(tmp2, mod->num_samples);
		    note.sample = (int)tmp2-1;
		    
		}
		if (tmpu8 & 64) {
		    fread(&tmp2, 1, 1, f);
		    note.volume = tmp2;
		} else {
		    note.volume = -1.0;
		}

		if (tmpu8 & 128) {
		    int argx, argy, code;

		    fread(&tmp2, 1, 1, f);
		    code = tmp2;
		    fread(&tmp2, 1, 1, f);
		    argx = (tmp2 >> 4) & 0x0F;
		    argy = tmp2 & 0x0F;
		    
		    if (code != 0) {
			InstallCommand(&note, code, argx, argy);
		    }
		}

		ARM_SetPatternNote(&mod->patterns[i], row, chan, note);

	    } else {
		row++;
	    }
	}
	fseek(f, filepos, SEEK_SET);
    }

    if (mod->num_channels > 16)
	mod->num_channels = 16;

    /* Read the default channel pan info, if present. */
/*
    if (!mono && has_default_pan) {
	for (i = 0; i < 32; i++) {
	    fread(&tmpu8, 1, 1, f);
	    tmpu8 &= 0x0F;
	    tmpu8 *= 16;
	    printf("Default panning for channel %i is %i\n",
		   i, tmpu8);
	    if (mod->default_pan[i] != -1) {
		mod->default_pan[i] = tmpu8;
	    }
	}
    } else {
	for (i = 0; i < 32; i++) {
	    mod->default_pan[i] = 0x7F;
	}
    }
*/

    /* Done! */

    fclose(f);
    return 0;

 failure:
    if (f != NULL) fclose(f);
    ARM_FreeModuleData(mod);
    return -1;
}

