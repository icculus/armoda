#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"

int ARM_AllocPatternData(ARM_Pattern* ptn, int rows, int cols)
{
    int r, c;
    ARM_Note n;
  
    memset(ptn, 0, sizeof (ARM_Pattern));
    ptn->notes = (struct ARM_Note **)malloc(cols * sizeof (struct ARM_Note *));
    if (ptn->notes == NULL)
	return -1;
    memset(ptn->notes, 0, cols * sizeof (struct ARM_Note *));
    ptn->rows = rows;
    ptn->cols = cols;
    for (c = 0; c < cols; c++) {

	ptn->notes[c] = (struct ARM_Note *)malloc(rows * sizeof (struct ARM_Note));
	if (ptn->notes[c] == NULL) {
	    ARM_FreePatternData(ptn);
	    return -1;
	}
	memset(ptn->notes[c], 0, rows * sizeof (struct ARM_Note));
    }

    memset(&n, 0, sizeof (n));
    n.sample = -1;
    n.volume = -1.0;
    for (r = 0; r < rows; r++) {
	for (c = 0; c < cols; c++) {
	    ARM_SetPatternNote(ptn, r, c, n);
	}
    }

    return 0;
}

void ARM_FreePatternData(ARM_Pattern* ptn)
{
    int c;

    if (ptn->notes != NULL) {
	for (c = 0; c < ptn->cols; c++) {
	    if (ptn->notes[c] != NULL) {
		free(ptn->notes[c]);
		ptn->notes[c] = NULL;
	    }
	}
	free(ptn->notes);
	ptn->notes = NULL;
    }
}

void ARM_SetPatternNote(ARM_Pattern* ptn, int row, int col, ARM_Note note)
{
    assert(row >= 0 && row < ptn->rows);
    assert(col >= 0 && col < ptn->cols);

    ptn->notes[col][row] = note;
}

ARM_Note* ARM_GetPatternNote(ARM_Pattern* ptn, int row, int col)
{
    if (row >= 0 && row < ptn->rows &&
	col >= 0 && col < ptn->cols)
	return &ptn->notes[col][row];

    return NULL;
}
