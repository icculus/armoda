#ifndef LOAD_DSM_H
#define LOAD_DSM_H

#include "tracker.h"

int ARM_LoadModule_DSM(ARM_Module* mod, const char *filename);
/* Loads a DSIK DSM file.
   Returns 0 on success, -1 on failure. */

#endif
