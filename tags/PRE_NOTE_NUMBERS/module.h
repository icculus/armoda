#ifndef MODULE_H
#define MODULE_H

#include "tracker.h"
#include "load_mod.h"
#include "load_s3m.h"
#include "load_dsm.h"

int ARM_LoadModule(ARM_Module* mod, const char *filename);
/* Loads a module in any supported format. */

void ARM_FreeModuleData(ARM_Module* mod);
/* Frees a module's data. */

int ARM_ConvertFinetuneToC4SPD(int finetune);

#endif
