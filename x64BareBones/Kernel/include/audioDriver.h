#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <stdint.h>

#define AUDIO_BACKEND_NONE        0u
#define AUDIO_BACKEND_PC_SPEAKER  1u
#define AUDIO_BACKEND_PCI_GENERIC 2u

typedef struct
{
    uint32_t sampleRate;
    uint32_t currentFrequency;
    uint8_t present;
    uint8_t initialized;
    uint8_t muted;
    uint8_t playing;
    uint8_t backend;
    uint8_t hasPciAudio;
    uint8_t channels;
    uint8_t bitsPerSample;
    uint8_t volume;
} audio_driver_status_t;

audio_driver_status_t audioGetStatus(void);

// Driver initialization functions
int audioInitDriver(void);
int audioDetectHardware(uint32_t mmioBase);
int audioResetHardware(void);
int audioConfigureFormat(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample);
int audioEnableInterrupts(void);

// Audio control functions
int audioPlayTone(uint32_t frequency);
int audioStopTone(void);
int audioMute(uint8_t enable);

#endif
