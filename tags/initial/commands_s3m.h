#ifndef COMMANDS_S3M_H
#define COMMANDS_S3M_H

#include "commands.h"

extern ARM_CommandType command_s3m_volslide_callbacks;
extern ARM_CommandType command_s3m_period_slide_up_callbacks;
extern ARM_CommandType command_s3m_period_slide_down_callbacks;
extern ARM_CommandType command_s3m_vibrato_callbacks;
extern ARM_CommandType command_s3m_tremolo_callbacks;
extern ARM_CommandType command_s3m_tremor_callbacks;
extern ARM_CommandType command_s3m_volslide_and_vibrato_callbacks;
extern ARM_CommandType command_s3m_period_and_volslide_callbacks;
extern ARM_CommandType command_s3m_retrig_and_volslide_callbacks;

void command_s3m_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_period_slide_up_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_period_slide_down_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_tremor_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_tremor_tick(struct ARM_Tracker* player, int c);
void command_s3m_tremor_cleanup(struct ARM_Tracker* player, int c);
void command_s3m_volslide_and_vibrato_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_period_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_mod_period_and_volslide_tick(struct ARM_Tracker* player, int c);
void command_mod_period_and_volslide_cleanup(struct ARM_Tracker* player, int c);
void command_s3m_retrig_and_volslide_init(struct ARM_Tracker* player, int c, int arg1, int arg2);
void command_s3m_tremolo_init(struct ARM_Tracker* player, int c, int arg1, int arg2);

#endif
