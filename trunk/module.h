#ifndef MODULE_H
#define MODULE_H

#include "tracker.h"
#include "load_mod.h"
#include "load_s3m.h"

void ARM_FreeModuleData(ARM_Module* mod);
/* Frees a module's data. */

int ARM_ConvertFinetuneToC4SPD(int finetune);

#endif
