#include <stdlib.h>
#include <stdio.h>
#include "tracker.h"

#define MOD2C_VERSION "mod2c v0, by John R. Hall <overcode@overcode.net>"

static void FreezeSamples(FILE* out_c, ARM_Module* mod)
{
    unsigned int s;
    int i, c;
    fprintf(out_c, "/* Frozen sample data. */\n");
    fprintf(out_c, "static uint8_t samples[] = {");
    c = 0;
    for (i = 0; i < mod->num_samples; i++) {
        for (s = 0; s < mod->samples[i].length; s++) {
            if (c%16 == 0)
                fprintf(out_c, "\n    ");
            c++;
            fprintf(out_c, "%3u,", (unsigned int)(mod->samples[i].data[s] * 128.0 + 128.0));
        }
    }
    fprintf(out_c, "0};\n\n");
}

static unsigned int LookupSampleNum(ARM_Module* mod, ARM_Sample* ptr)
{
    return (unsigned int)(ptr - &mod->samples[0]) / sizeof(ARM_Sample);
}

static void FreezeCommand(FILE* out_c, ARM_Command* cmd)
{
    fprintf(out_c, "{ %i, %i, %i }",
            cmd->cmd, cmd->arg1, cmd->arg2);
}

static void FreezeNote(FILE* out_c, ARM_Note* note)
{
    fprintf(out_c, "{ %i, %f, %f, %i, ",
            note->sample,
            note->volume,
            note->period,
            note->trigger);
    FreezeCommand(out_c, &note->cmd);
    fprintf(out_c, " }");
}

static void FreezePatterns(FILE* out_c, ARM_Module* mod)
{
    int p, r, c;

    fprintf(out_c, "/* Frozen pattern data. */\n");
    fprintf(out_c, "static ARM_Note patterns[] = {\n");
    for (p = 0; p < mod->num_patterns; p++) {
        for (r = 0; r < 64; r++) {
            for (c = 0; c < mod->num_channels; c++) {
                ARM_Note* note;
                note = ARM_GetPatternNote(&mod->patterns[p], r, c);
                FreezeNote(out_c, note);
                fprintf(out_c, ", ");
            }
            fprintf(out_c, "\n");
        }
    }
    fprintf(out_c, "{}\n");
    fprintf(out_c, "};\n\n");
}

static void FreezeMisc(FILE* out_c, ARM_Module* mod)
{
    int i;
    fprintf(out_c, "/* Frozen order list. */\n");
    fprintf(out_c, "static int OrderList[] = { ");
    for (i = 0; i < mod->num_order; i++)
        fprintf(out_c, "%i,", mod->order[i]);
    fprintf(out_c, "0 };\n\n");
    fprintf(out_c, "/* Frozen panning list. */\n");
    fprintf(out_c, "static int DefaultPanning[] = { ");
    for (i = 0; i < mod->num_channels; i++)
        fprintf(out_c, "%i,", mod->default_pan[i]);
    fprintf(out_c, "0 };\n\n");
    
}

static void WriteLoader(FILE* out_c, ARM_Module* mod, char *mod_identifier)
{
    int i, s = 0;
    fprintf(out_c, "/* Loader. */\n");

    fprintf(out_c, "static void UnfreezePatterns(ARM_Module* mod)\n");
    fprintf(out_c, "{\n");
    fprintf(out_c, "    int p, r, c;\n");
    fprintf(out_c, "    ARM_Note* in = patterns;\n");
    fprintf(out_c, "    for (p = 0; p < %i; p++) {\n", mod->num_patterns);
    fprintf(out_c, "        ARM_AllocPatternData(&mod->patterns[p], 64, %i);\n", mod->num_channels);
    fprintf(out_c, "        for (r = 0; r < 64; r++) {\n");
    fprintf(out_c, "                for (c = 0; c < %i; c++) {\n", mod->num_channels);
    fprintf(out_c, "                    ARM_SetPatternNote(&mod->patterns[p], r, c, *in);\n");
    fprintf(out_c, "                    in++;\n");
    fprintf(out_c, "                }\n");
    fprintf(out_c, "            }\n");
    fprintf(out_c, "    }\n");
    fprintf(out_c, "}\n\n");

    fprintf(out_c, "int LoadEmbeddedModule_%s(ARM_Module* mod)\n", mod_identifier);
    fprintf(out_c, "{\nint i, s;\n");
    fprintf(out_c, "    memset(mod, 0, sizeof (ARM_Module));\n");
    fprintf(out_c, "    mod->title = strdup(\"Embedded module (%s)\");\n", mod_identifier);
    fprintf(out_c, "    if (mod->title == NULL) return -1;\n");
    fprintf(out_c, "    mod->num_channels = %i;\n", mod->num_channels);
    fprintf(out_c, "    mod->num_samples = %i;\n", mod->num_samples);
    fprintf(out_c, "    mod->num_order = %i;\n", mod->num_order);
    fprintf(out_c, "    mod->num_patterns = %i;\n", mod->num_patterns);
    fprintf(out_c, "    mod->flags = %u;\n", mod->flags);
    fprintf(out_c, "    mod->master_volume = %i;\n", mod->master_volume);
    fprintf(out_c, "    mod->global_volume = %i;\n", mod->global_volume);
    fprintf(out_c, "    mod->initial_speed = %i;\n", mod->initial_speed);
    fprintf(out_c, "    mod->initial_bpm = %i;\n", mod->initial_bpm);
    fprintf(out_c, "    mod->samples = (ARM_Sample*)mallocsafe(%i*sizeof(ARM_Sample));\n",
            mod->num_samples);
    fprintf(out_c, "    if (mod->samples == NULL) goto boom; memset(mod->samples, 0, %i*sizeof(ARM_Sample));\n", mod->num_samples);
    fprintf(out_c, "    mod->patterns = (ARM_Pattern*)mallocsafe(%i*sizeof(ARM_Pattern));\n",
            mod->num_patterns);
    fprintf(out_c, "    if (mod->patterns == NULL) goto boom; memset(mod->patterns, 0, %i*sizeof(ARM_Pattern));\n", mod->num_patterns);
    fprintf(out_c, "    mod->default_pan = (int*)mallocsafe(%i*sizeof(int));\n", mod->num_channels);
    fprintf(out_c, "    memcpy(mod->default_pan, DefaultPanning, %i * sizeof(int));\n", mod->num_channels);
    fprintf(out_c, "    mod->order = (int*)mallocsafe(%i*sizeof(int));\n", mod->num_order);
    fprintf(out_c, "    memcpy(mod->order, OrderList, %i * sizeof(int));\n", mod->num_order);
    for (i = 0, s = 0; i < mod->num_samples; s += mod->samples[i].length, i++) {
        fprintf(out_c, "    mod->samples[%i].length = %u;\n", i, mod->samples[i].length);
        fprintf(out_c, "    mod->samples[%i].volume = %f;\n", i, mod->samples[i].volume);
        fprintf(out_c, "    mod->samples[%i].repeat_ofs = %i;\n", i, mod->samples[i].repeat_ofs);
        fprintf(out_c, "    mod->samples[%i].repeat_len = %i;\n", i, mod->samples[i].repeat_len);
        fprintf(out_c, "    mod->samples[%i].repeat_enabled = %i;\n", i, mod->samples[i].repeat_enabled);
        fprintf(out_c, "    mod->samples[%i].c4spd = %i;\n", i, mod->samples[i].c4spd);
        fprintf(out_c, "    mod->samples[%i].data = (float*)mallocsafe(%i*sizeof(float));\n",
                i, mod->samples[i].length);
        fprintf(out_c, "    if (mod->samples[%i].data == NULL) goto boom;\n", i);
        fprintf(out_c, "    for (s = 0; s < %i; s++)\n", mod->samples[i].length);
        fprintf(out_c, "        mod->samples[%i].data[s] = ((float)samples[s+%i] - 128.0) / 128.0;\n", i, s);
    }
    fprintf(out_c, "    UnfreezePatterns(mod);\n");
    fprintf(out_c, "    return 0;\n");
    fprintf(out_c, "boom: ARM_FreeModuleData(mod); return -1;\n");
    fprintf(out_c, "}\n");
}

int main(int argc, char *argv[])
{
    int i;
    char *mod_identifier;
    char *mod_filename;
    ARM_Module mod;
    FILE *out_c = NULL, *out_h = NULL;
    char *out_name;

    if (argc < 3) {
        fprintf(stderr, "Usage: mod2c <identifier> <filename>\n");
        return EXIT_FAILURE;
    }

    mod_identifier = argv[1];
    mod_filename = argv[2];
    
    /* Load the module. */
    if (ARM_LoadModule(&mod, mod_filename) < 0) {
        fprintf(stderr, "Unable to load '%s'.\n", mod_filename);
        goto cleanup;
    }

    printf("Dumping '%s' to C.\n", argv[2]);
    
    /* Open the output C and header files. */
    out_name = (char *)malloc(strlen(mod_filename)+3);
    if (out_name == NULL) abort();
    strcpy(out_name, mod_filename);
    strcat(out_name, ".c");
    out_c = fopen(out_name, "w");
    out_name[strlen(out_name)-1] = 'h';
    out_h = fopen(out_name, "w");
    free(out_name);
    if (out_c == NULL || out_h == NULL) {
        fprintf(stderr, "Unable to open output files.\n");
        if (out_c != NULL) fclose(out_c);
        if (out_h != NULL) fclose(out_h);
            goto cleanup;
    }

    /* Start the header file. Print some basic info. */
    fprintf(out_h, "#ifndef MODULE_%s_H\n#define MODULE_%s_H\n\n", mod_identifier, mod_identifier);

    fprintf(out_h, "/* Header file for module '%s', converted by %s. */\n\n",
            mod_filename,
            MOD2C_VERSION);
    fprintf(out_h, "/* Module contains %i samples, %i channels, %i patterns, %i orders. */\n\n",
            mod.num_samples,
            mod.num_channels,
            mod.num_patterns,
            mod.num_order);

    fprintf(out_h, "#include \"tracker.h\"\n\n");

    /* Write extern references to header file. */
    fprintf(out_h, "int LoadEmbeddedModule_%s(ARM_Module* mod);\n\n", mod_identifier);

    /* Close out the header. */
    fprintf(out_h, "#endif\n");

    fprintf(out_c, "#include \"tracker.h\"\n\n");

    /* Write sample data. */
    FreezeMisc(out_c, &mod);
    FreezeSamples(out_c, &mod);
    FreezePatterns(out_c, &mod);
    WriteLoader(out_c, &mod, mod_identifier);

 cleanup:
    /* Free everything. */
    ARM_FreeModuleData(&mod);

    /* Close files. */
    if (out_h != NULL) fclose(out_h);
    if (out_c != NULL) fclose(out_c);

    return EXIT_SUCCESS;
}
