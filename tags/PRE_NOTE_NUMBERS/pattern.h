#ifndef PATTERN_H
#define PATTERN_H

#include <stdint.h>
#include "commands.h"
#include "tracker.h"

int ARM_AllocPatternData(struct ARM_Pattern* ptn, int rows, int cols);
/* Creates a pattern with the given number of rows and columns.
   Initializes the pattern to all zeros.
   Returns -1 on failure, 0 on success. */

void ARM_FreePatternData(struct ARM_Pattern* ptn);
/* Frees a pattern's memory. */

void ARM_SetPatternNote(struct ARM_Pattern* ptn, int row, int col, ARM_Note note);
/* Sets the value of the note at the given position.
   Out of range positions may cause an assert() failure, or
   may simply be ignored. Don't. */

ARM_Note* ARM_GetPatternNote(struct ARM_Pattern* ptn, int row, int col);
/* Returns a pointer to the note at the given row and column.
   Returns NULL if out of range. */

#endif
