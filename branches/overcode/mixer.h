#ifndef MIXER_H
#define MIXER_H

typedef struct Mixer {

    int (*Render)(struct Mixer* mixer, unsigned int frames, float divisor);
    /* Renders the given number of frames (one frame is a sample for each channel).
       divisor is a global volume divisor; every voice's volume is divided by
       this amount prior to mixing. */

    /*
     * Patch management
     */

    int (*LoadPatch)(struct Mixer* mixer, float* data, unsigned int length);
    /* Prepares a patch for use. Returns a patch identifier on success, -1 on failure.
       Makes a copy of the patch's data; the caller may free the data after loading a patch. */

    int (*UnloadPatch)(struct Mixer* mixer, int patch);
    /* Removes a patch from the mixer. */

    /*
     * Voice management
     */
    
    int (*AllocVoice)(struct Mixer* mixer, int priority);
    /* Allocates a voice. If insufficient voices are available, temporarily
       disables the existing voice with the lowest priority.
       Returns the voice number on success, or -1 on failure. */

    int (*FreeVoice)(struct Mixer* mixer, int voice);
    /* Releases a voice. It is an error to make further calls on a voice
       that has been freed. There is no need to stop playback before
       freeing a voice. */

    int (*IsVoiceLive)(struct Mixer* mixer, int voice);
    /* Returns 1 if the given voice is live (present and working),
       or 0 if it has been evicted due to low priority.
       Calls on evicted voices generally succeed, unless they would fail
       for other reasons, but they do not play, and their positions are never
       updated.
       It is an error to call this function on a voice that has not been allocated. */

    int (*SetVoicePatch)(struct Mixer* mixer, int voice, int patch);
    /* Selects a patch for a voice to use.
       Un-triggers the voice, clears the loop position, and resets playback
       position to zero. */
    
    int (*SetVoiceVolume)(struct Mixer* mixer, int voice, float volume);
    /* Sets a voice's volume, 0..1. */

    int (*SetVoiceRate)(struct Mixer* mixer, int voice, float rate);
    /* Sets a voice's playback rate, in Hz. */

    int (*SetVoiceLoopPos)(struct Mixer* mixer, int voice, unsigned int loop_start, unsigned int loop_len);
    /* Sets a voice's loop start position. When playback reaches the end
       of the sample, playback will resume from the loop start point.
       This interface is expected to change fairly soon. */

    int (*SetVoiceLoopMode)(struct Mixer* mixer, int voice, int enabled);
    /* Enables or disables looping on a voice.
       This interface is expected to change fairly soon. */

    int (*SetVoicePos)(struct Mixer* mixer, int voice, unsigned int pos);
    /* Sets a voice's playback position. */

    int (*SetVoiceBalance)(struct Mixer* mixer, int voice, float panning);
    /* Sets a voice's panning position, -1..1. */

    int (*TriggerVoice)(struct Mixer* mixer, int voice);
    /* Begins playback on a voice from position zero. */
    
    int (*StopVoice)(struct Mixer* mixer, int voice);
    /* Stops playback on a voice. Does not reset its playback position. */

    void *private;
    /* Private mixer-specific data. */

} Mixer;

#endif
