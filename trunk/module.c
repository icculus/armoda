#include <stdlib.h>
#include "module.h"


void ARM_FreeSample(ARM_Sample* sam)
{
    if (sam != NULL) {
	free(sam->name);
	free(sam->data);
    }
}


void ARM_FreeModuleData(ARM_Module* mod)
{
    int i;

    free(mod->title);
    if (mod->samples != NULL) {
	for (i = 0; i < mod->num_samples; i++) {
	    ARM_FreeSample(&mod->samples[i]);
	}
    }
    free(mod->samples);
    free(mod->order);
    if (mod->patterns != NULL) {
	for (i = 0; i < mod->num_patterns; i++) {
	    ARM_FreePatternData(&mod->patterns[i]);
	}
    }
    free(mod->patterns);
    free(mod->default_pan);
    memset(mod, 0, sizeof (ARM_Module));
}


typedef int (*LoaderProc)(ARM_Module* mod, const char* filename);

int ARM_LoadModule(ARM_Module* mod, const char* filename)
{
    char *ext;
    int i;
    const char *extensions[] =
	{ ".mod",
	  ".s3m",
	  ".dsm",
	  NULL };
    LoaderProc loaders[] =
	{ ARM_LoadModule_MOD,
	  ARM_LoadModule_S3M,
	  ARM_LoadModule_DSM,
	  NULL };

    /* Find the file extension. */
    ext = strrchr(filename, '.');
    if (ext == NULL)
	return -1;
    
    /* Look up the extension. */
    for (i = 0; extensions[i] != NULL; i++) {
	if (!strcasecmp(ext, extensions[i]))
	    return loaders[i](mod, filename);
    }
    
    return -1;
}
