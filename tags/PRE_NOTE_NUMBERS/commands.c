#include <stdio.h>
#include <stdlib.h>
#include "commands.h"
#include "commands_mod.h"
#include "commands_s3m.h"


static ARM_CommandType *command_callbacks[] = {

    &command_null_callbacks,
    
    /* MOD commands */
    &command_mod_set_speed_callbacks,
    &command_mod_set_tempo_callbacks,
    &command_mod_goto_callbacks,
    &command_mod_volslide_callbacks,
    &command_mod_fine_volslide_callbacks,
    &command_mod_period_slide_callbacks,
    &command_mod_slide_to_note_callbacks,
    &command_mod_arpeggio_callbacks,
    &command_mod_vibrato_callbacks,
    &command_mod_tremolo_callbacks,
    &command_mod_offset_callbacks,
    &command_mod_set_volume_callbacks,
    &command_mod_volslide_and_vibrato_callbacks,
    &command_mod_period_and_volslide_callbacks,
    &command_mod_fine_period_slide_callbacks,
    &command_mod_set_c4spd_callbacks,
    &command_mod_set_pattern_loop_callbacks,
    &command_mod_do_pattern_loop_callbacks,
    &command_mod_cut_channel_callbacks,
    &command_mod_delay_trigger_callbacks,
    &command_mod_delay_pattern_callbacks,
    &command_mod_retrigger_callbacks,
    &command_mod_set_channel_pan_callbacks,
    
    /* S3M commands */
    &command_s3m_volslide_callbacks,
    &command_s3m_period_slide_up_callbacks,
    &command_s3m_period_slide_down_callbacks,
    &command_s3m_vibrato_callbacks,
    &command_s3m_tremolo_callbacks,
    &command_s3m_tremor_callbacks,
    &command_s3m_volslide_and_vibrato_callbacks,
    &command_s3m_period_and_volslide_callbacks,
    &command_s3m_retrig_and_volslide_callbacks,

    NULL
};


ARM_CommandType* ARM_GetCallbacksForNum(unsigned int cmd)
{
    return command_callbacks[cmd];
}

unsigned int ARM_GetNumForCallbacks(ARM_CommandType* callbacks)
{
    unsigned int i;
    
    for (i = 0; command_callbacks[i] != NULL; i++) {
	if (command_callbacks[i] == callbacks)
	    return i;
    }

    fprintf(stderr, "WARNING! Unable to lookup command number for %p.\n", callbacks);

    return 0; /* null callbacks; no real harm */
}
