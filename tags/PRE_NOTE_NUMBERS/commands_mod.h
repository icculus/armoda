#ifndef COMMANDS_MOD_H
#define COMMANDS_MOD_H

#include "commands.h"

float ARM_MOD_CalcVibrato(struct ARM_Tracker* player, int c);
float ARM_MOD_CalcTremolo(struct ARM_Tracker* player, int c);

extern ARM_CommandType command_mod_set_speed_callbacks;
extern ARM_CommandType command_mod_set_tempo_callbacks;
extern ARM_CommandType command_mod_goto_callbacks;
extern ARM_CommandType command_mod_volslide_callbacks;
extern ARM_CommandType command_mod_fine_volslide_callbacks;
extern ARM_CommandType command_mod_period_slide_callbacks;
extern ARM_CommandType command_mod_slide_to_note_callbacks;
extern ARM_CommandType command_mod_arpeggio_callbacks;
extern ARM_CommandType command_mod_vibrato_callbacks;
extern ARM_CommandType command_mod_tremolo_callbacks;
extern ARM_CommandType command_mod_offset_callbacks;
extern ARM_CommandType command_mod_set_volume_callbacks;
extern ARM_CommandType command_mod_volslide_and_vibrato_callbacks;
extern ARM_CommandType command_mod_period_and_volslide_callbacks;
extern ARM_CommandType command_mod_fine_period_slide_callbacks;
extern ARM_CommandType command_mod_set_c4spd_callbacks;
extern ARM_CommandType command_mod_set_pattern_loop_callbacks;
extern ARM_CommandType command_mod_do_pattern_loop_callbacks;
extern ARM_CommandType command_mod_cut_channel_callbacks;
extern ARM_CommandType command_mod_delay_trigger_callbacks;
extern ARM_CommandType command_mod_delay_pattern_callbacks;
extern ARM_CommandType command_mod_retrigger_callbacks;
extern ARM_CommandType command_mod_set_channel_pan_callbacks;
/* Be sure to add any more callbacks to commands.c table */

void command_mod_arpeggio_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_arpeggio_cleanup(struct ARM_Tracker* player, int c);
void command_mod_arpeggio_tick(struct ARM_Tracker* player, int c);
void command_mod_period_slide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_period_slide_tick(struct ARM_Tracker* player, int c);
void command_mod_period_slide_cleanup(struct ARM_Tracker* player, int c);
void command_mod_slide_to_note_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_vibrato_tick(struct ARM_Tracker* player, int c);
void command_mod_vibrato_cleanup(struct ARM_Tracker* player, int c);
void command_mod_period_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_period_and_volslide_tick(struct ARM_Tracker* player, int c);
void command_mod_period_and_volslide_cleanup(struct ARM_Tracker* player, int c);
void command_mod_volslide_and_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_volslide_and_vibrato_tick(struct ARM_Tracker* player, int c);
void command_mod_volslide_and_vibrato_cleanup(struct ARM_Tracker* player, int c);
void command_mod_tremolo_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_tremolo_tick(struct ARM_Tracker* player, int c);
void command_mod_tremolo_cleanup(struct ARM_Tracker* player, int c);
void command_mod_offset_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_volslide_tick(struct ARM_Tracker* player, int c);
void command_mod_volslide_cleanup(struct ARM_Tracker* player, int c);
void command_mod_goto_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_volume_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_speed_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_tempo_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_fine_period_slide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_c4spd_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_pattern_loop_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_do_pattern_loop_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_set_channel_pan_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_retrigger_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_retrigger_tick(struct ARM_Tracker* player, int c);
void command_mod_retrigger_cleanup(struct ARM_Tracker* player, int c);
void command_mod_fine_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_cut_channel_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_cut_channel_tick(struct ARM_Tracker* player, int c);
void command_mod_delay_trigger_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_delay_trigger_tick(struct ARM_Tracker* player, int c);
void command_mod_delay_pattern_init(struct ARM_Tracker* player, int c, int arg1, int arg2);

#endif
