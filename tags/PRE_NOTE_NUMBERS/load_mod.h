#ifndef LOAD_MOD_H
#define LOAD_MOD_H

#include "tracker.h"

int ARM_LoadModule_MOD(ARM_Module* mod, const char *filename);
/* Loads a Protracker or similar MOD file.
   Supports several structurally similar formats.
   Returns 0 on success, -1 on failure. */

int ARM_MOD_InstallCommand(ARM_Note* note, uint8_t code, uint8_t argx, uint8_t argy);

#endif
