/*
 * DSIK DSM loader.
 * For lack of other documentation (grr!) I referred heavily
 * to the MikMod DSM loader. However, I wrote this loader from scratch. -jrh
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "tracker.h"
#include "commands.h"
#include "commands_mod.h"
#include "osutil.h"



/* Reads a RIFF ID and length into the given buffers.
   Returns 0 on success, -1 on failure. */
static int ReadRIFFTag(FILE* f, char* buf, unsigned int* length, unsigned int* next)
{
    uint32_t len;
    if (fread(buf, 4, 1, f) != 1)
	return -1;
    if (fread(&len, 4, 1, f) != 1)
	return -1;
    *length = SWAP_LE_32(len);
    *next = ftell(f) + *length;
    return 0;
}

int ARM_LoadModule_DSM(ARM_Module* mod, const char *filename)
{
    char riff_tag[5];
    unsigned int riff_length;
    unsigned int riff_next;
    FILE *f;
    uint16_t tmp16;
    uint32_t tmp32;
    uint8_t tmp8;
    uint16_t dsm_version;
    uint16_t dsm_flags;
    uint16_t dsm_num_orders;
    uint16_t dsm_num_samples;
    uint16_t dsm_num_patterns;
    uint16_t dsm_num_channels;
    uint8_t dsm_global_volume;
    uint8_t dsm_master_volume;
    uint8_t dsm_initial_speed;
    uint8_t dsm_initial_bpm;
    uint8_t dsm_panning[16];
    uint8_t dsm_orders[128];
    int pattern, sample;
    int c;
    unsigned int u;
    int i;

    /* Clear out the module. */
    memset(mod, 0, sizeof (ARM_Module));

    /* Open the file. */
    f = fopen(filename, "rb");
    if (f == NULL)
	return -1;

    riff_tag[4] = '\0';

    /* Check the RIFF header. We don't really care about the length. */
    if (ReadRIFFTag(f, riff_tag, &riff_length, &riff_next) != 0) goto failure;
    if (strcmp(riff_tag, "RIFF")) {
	fprintf(stderr, "DSM loader: not a RIFF file!\n");
    }

    /* Ok, some weirdness here. Not really a RIFF header. */
    if (fread(riff_tag, 4, 1, f) != 1) goto failure;
    if (strcmp(riff_tag, "DSMF")) {
	fprintf(stderr, "DSM loader: not a DSM file!\n");
	goto failure;
    }

    /* Look for the SONG header. */
    if (ReadRIFFTag(f, riff_tag, &riff_length, &riff_next) != 0) goto failure;
    if (strcmp(riff_tag, "SONG")) {
	fprintf(stderr, "DSM loader: expected SONG block!\n");
	goto failure;
    }

    /* Read the song info header. */
    mod->title = (char *)mallocsafe(29);
    memset(mod->title, 0, 29);
    fread(mod->title, 28, 1, f);

    fread(&tmp16, 2, 1, f);
    dsm_version = SWAP_LE_16(tmp16);

    fread(&tmp16, 2, 1, f);
    dsm_flags = SWAP_LE_16(tmp16);

    fread(&tmp32, 4, 1, f);
    
    fread(&tmp16, 2, 1, f);
    dsm_num_orders = SWAP_LE_16(tmp16);
    
    fread(&tmp16, 2, 1, f);
    dsm_num_samples = SWAP_LE_16(tmp16);
    
    fread(&tmp16, 2, 1, f);
    dsm_num_patterns = SWAP_LE_16(tmp16);

    fread(&tmp16, 2, 1, f);
    dsm_num_channels = SWAP_LE_16(tmp16);
    
    fread(&dsm_global_volume, 1, 1, f);

    fread(&dsm_master_volume, 1, 1, f);
    
    fread(&dsm_initial_speed, 1, 1, f);

    fread(&dsm_initial_bpm, 1, 1, f);

    fread(dsm_panning, 16, 1, f);
    fread(dsm_orders, 128, 1, f);

    /* Fill in the module header. */
    mod->num_channels = dsm_num_channels;
    mod->num_order = dsm_num_orders;
    mod->global_volume = dsm_global_volume;
    mod->master_volume = dsm_master_volume;
    mod->initial_speed = dsm_initial_speed;
    mod->initial_bpm = dsm_initial_bpm;
    mod->num_patterns = dsm_num_patterns;
    mod->num_samples = dsm_num_samples;

    /* Set up order list. */    
    if (mod->num_order > 128)
	mod->num_order = 128;
    mod->order = (int *)mallocsafe((mod->num_order + 1) * sizeof (int));
    for (i = 0; i < mod->num_order; i++) {
	mod->order[i] = dsm_orders[i];
    }
    mod->order[i] = 255; /* end of song, just in case */

    /* Set up default panning info. */
    mod->default_pan = (int*)mallocsafe(mod->num_channels * sizeof (int));
    for (i = 0; i < mod->num_channels; i++) {
	mod->default_pan[i] = (dsm_panning[i] == 0xA4 ?
			       128 :   /* surround not supported yet, so center it */
			       (dsm_panning[i] < 0x80 ? (dsm_panning[i] << 1) : 255));
	/* FIXME: This is probably not right. Clean up. */
	printf("panning %i\n", mod->default_pan[i]);
    }

    /* Read blocks. */
    mod->patterns = (ARM_Pattern*)mallocsafe(mod->num_patterns * sizeof (ARM_Pattern));
    mod->samples = (ARM_Sample*)mallocsafe(mod->num_samples * sizeof (ARM_Sample));
    sample = 0;
    pattern = 0;

    while (pattern < mod->num_patterns || sample < mod->num_samples) {

	/* Seek to the next block. */
	fseek(f, riff_next, SEEK_SET);

	if (ReadRIFFTag(f, riff_tag, &riff_length, &riff_next) != 0)
	    break;

	if (!strcmp(riff_tag, "INST")) {
	    
	    ARM_Sample* sam;
	    uint16_t inst_flags;
	    int is_signed;
	    void *buf;

	    printf("Reading sample %i.\n", sample);

	    sam = &mod->samples[sample];
	    memset(sam, 0, sizeof (ARM_Sample));

	    /* Read the sample header. */
	    sam->filename = (char *)mallocsafe(14);
	    memset(sam->filename, 0, 14);
	    fread(sam->filename, 13, 1, f);
	    fread(&tmp16, 2, 1, f);
	    inst_flags = SWAP_LE_16(tmp16);
	    if (inst_flags & 1)
		sam->repeat_enabled = 1;
	    else
		sam->repeat_enabled = 0;
	    if (inst_flags & 2)
		is_signed = 1;
	    else
		is_signed = 0;
	    fread(&tmp8, 1, 1, f);
	    sam->volume = (float)tmp8;
	    fread(&tmp32, 4, 1, f);
	    sam->length = SWAP_LE_32(tmp32);
	    fread(&tmp32, 4, 1, f);
	    sam->repeat_ofs = SWAP_LE_32(tmp32);
	    fread(&tmp32, 4, 1, f);
	    sam->repeat_len = SWAP_LE_32(tmp32) - sam->repeat_ofs;
	    fread(&tmp32, 4, 1, f);
	    fread(&tmp16, 2, 1, f);
	    sam->c4spd = SWAP_LE_16(tmp16);
	    fread(&tmp16, 2, 1, f);
	    sam->name = (char *)mallocsafe(29);
	    memset(sam->name, 0, 29);
	    fread(sam->name, 28, 1, f);

	    sam->data = (float *)mallocsafe(sam->length * sizeof (float));
	    buf = malloc(sam->length);
	    fread(buf, sam->length, 1, f);

	    if (is_signed) {
		int8_t *sbuf = (int8_t *)buf;
		for (u = 0; u < sam->length; u++) {
		    sam->data[u] = (float)sbuf[u] / 128.0;
		}
	    } else {
		uint8_t *ubuf = (uint8_t *)buf;
		for (u = 0; u < sam->length; u++) {
		    sam->data[u] = (((float)ubuf[u] / 255.0) - 0.5) * 2.0;
		}
	    }
	    
	    free(buf);
	    
	    /* Done with this sample. */
	    sample++;

	} else if (!strcmp(riff_tag, "PATT")) {

	    int row;
	    ARM_Pattern *pat;

	    printf("Reading pattern %i.\n", pattern);

	    pat = &mod->patterns[pattern];

	    /* Allocate the pattern. */
	    if (ARM_AllocPatternData(pat, 64, mod->num_channels) != 0) goto failure;

	    /* Read pattern data. */
	    row = 0;
	    while (row < 64) {
		uint8_t present;
		uint8_t raw_note = 0, raw_ins = 0, raw_vol = 0, raw_cmd = 0, raw_inf = 0;
		ARM_Note* note;

		fread(&present, 1, 1, f);
		if (present) {

		    note = ARM_GetPatternNote(pat, row, present & 0x0F);

		    /* Note present? */
		    if (present & 0x80) {
			fread(&raw_note, 1, 1, f);
			if (raw_note != 255) {
			    note->period = 0; /* (float)note_to_period[raw_note];*/
			    note->trigger = 1;
			}
		    } else {
			note->period = 0.0;
			note->trigger = 0;
		    }

		    /* Instrument present? */
		    if (present & 0x40) {
			fread(&raw_ins, 1, 1, f);
			UPPER_CLAMP(raw_ins, mod->num_samples);
			note->sample = (int)raw_ins-1;
		    } else {
			note->sample = -1;
		    }

		    /* Volume present? */
		    if (present & 0x20) {
			fread(&raw_vol, 1, 1, f);
			note->volume = (float)raw_vol;
		    } else {
			note->volume = -1.0;
		    }

		    /* Effect present? */
		    if (present & 0x10) {
			int argx, argy;
			fread(&raw_cmd, 1, 1, f);
			fread(&raw_inf, 1, 1, f);
			argx = (raw_inf & 0xF0) >> 4;
			argy = (raw_inf & 0x0F);
			ARM_MOD_InstallCommand(note, raw_cmd, argx, argy);			
		    } else {
			memset(&note->cmd, 0, sizeof (ARM_Command));
		    }
		    
		} else {
		    row++;
		}
	    }

	    /* Done with this pattern. */
	    pattern++;
	} else {
	    break;
	}

    }	
    
    fclose(f);
    return 0;

 failure:
    ARM_FreeModuleData(mod);
    fclose(f);
    return -1;
}
