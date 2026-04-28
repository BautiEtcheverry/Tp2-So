#include <audioDriver.h>
#include <libasm.h>
#include <stdint.h>

#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL2_PORT 0x42
#define PIT_GATE_PORT 0x61
#define PIT_BASE_FREQUENCY 1193182u

#define AUDIO_MIN_FREQUENCY 20u
#define AUDIO_MAX_FREQUENCY 20000u

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA 0x0CFC

#define AUDIO_MMIO_DEFAULT_BASE 0x00000000
#define AUDIO_DEFAULT_SAMPLE_RATE 48000
#define AUDIO_DEFAULT_CHANNELS 2
#define AUDIO_DEFAULT_BITS 16
#define AUDIO_DEFAULT_VOLUME 100

typedef struct
{
    uint8_t present;
    uint8_t initialized;
    uint8_t muted;
    uint8_t backend;
    uint8_t hasPciAudio;
    uint8_t playing;
    uint32_t mmioBase;
    uint32_t sampleRate;
    uint32_t currentFrequency;
    uint16_t currentDivisor;
    uint8_t channels;
    uint8_t bitsPerSample;
    uint8_t volume;
} audio_driver_ctx_t;

static audio_driver_ctx_t audioCtx;

static inline void outl(uint16_t port, uint32_t value)
{
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void pitProgramChannel2(uint16_t divisor)
{
    if (divisor == 0)
        return;

    outb(PIT_COMMAND_PORT, 0xB6);
    outb(PIT_CHANNEL2_PORT, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL2_PORT, (uint8_t)((divisor >> 8) & 0xFFu));
}

static void speakerSetEnabled(uint8_t enable)
{
    uint8_t gate = inb(PIT_GATE_PORT);
    if (enable)
        gate = (uint8_t)(gate | 0x03u);
    else
        gate = (uint8_t)(gate & ~(uint8_t)0x03u);
    outb(PIT_GATE_PORT, gate);
}

static void audioSilenceSpeaker(void)
{
    outb(PIT_COMMAND_PORT, 0xB6);
    outb(PIT_CHANNEL2_PORT, 0);
    outb(PIT_CHANNEL2_PORT, 0);
    speakerSetEnabled(0);
    audioCtx.playing = 0;
}

static uint16_t audioFrequencyToDivisor(uint32_t frequency)
{
    if (frequency == 0)
        return 0;

    if (frequency < AUDIO_MIN_FREQUENCY)
        frequency = AUDIO_MIN_FREQUENCY;
    if (frequency > AUDIO_MAX_FREQUENCY)
        frequency = AUDIO_MAX_FREQUENCY;

    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xFFFFu)
        divisor = 0xFFFFu;
    return (uint16_t)divisor;
}

static void audioApplyOutputState(void)
{
    if (!audioCtx.present)
        return;
    if (audioCtx.backend != AUDIO_BACKEND_PC_SPEAKER)
        return;

    if (audioCtx.currentDivisor == 0 || audioCtx.muted || audioCtx.volume == 0)
    {
        audioSilenceSpeaker();
        return;
    }

    pitProgramChannel2(audioCtx.currentDivisor);
    speakerSetEnabled(1);
    audioCtx.playing = 1;
}

static inline uint32_t pciConfigRead32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t address = 0x80000000u |
                       ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) |
                       ((uint32_t)function << 8) |
                       (offset & 0xFCu);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static inline uint8_t pciConfigRead8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t value = pciConfigRead32(bus, device, function, offset);
    return (uint8_t)((value >> ((offset & 0x3u) * 8u)) & 0xFFu);
}

static uint32_t pciGetFirstUsableBar(uint8_t bus, uint8_t device, uint8_t function)
{
    uint32_t resolved = 0;

    for (uint8_t bar = 0; bar < 6 && resolved == 0; ++bar)
    {
        uint8_t offset = (uint8_t)(0x10 + bar * 4);
        uint32_t raw = pciConfigRead32(bus, device, function, offset);

        if (raw == 0 || raw == 0xFFFFFFFFu)
            continue;

        if ((raw & 0x1u) == 0)
        {
            resolved = raw & 0xFFFFFFF0u;
            if ((raw & 0x6u) == 0x4u)
                ++bar;
        }
        else
        {
            resolved = raw & 0xFFFFFFFCu;
        }
    }

    return resolved;
}

static int pciFindAudioDevice(uint32_t *mmioBaseOut)
{
    for (uint16_t bus = 0; bus < 256; ++bus)
    {
        for (uint8_t device = 0; device < 32; ++device)
        {
            uint32_t vendorDevice = pciConfigRead32((uint8_t)bus, device, 0, 0x00);
            if (vendorDevice == 0xFFFFFFFFu)
                continue;

            uint8_t functions = 1;
            uint8_t headerType = pciConfigRead8((uint8_t)bus, device, 0, 0x0E);
            if ((headerType & 0x80u) != 0)
                functions = 8;

            for (uint8_t function = 0; function < functions; ++function)
            {
                vendorDevice = pciConfigRead32((uint8_t)bus, device, function, 0x00);
                if (vendorDevice == 0xFFFFFFFFu)
                    continue;

                uint32_t classReg = pciConfigRead32((uint8_t)bus, device, function, 0x08);
                uint8_t classCode = (uint8_t)((classReg >> 24) & 0xFFu);
                uint8_t subclass = (uint8_t)((classReg >> 16) & 0xFFu);

                if (classCode != 0x04u)
                    continue;
                if (subclass != 0x01u && subclass != 0x03u)
                    continue;

                uint32_t base = pciGetFirstUsableBar((uint8_t)bus, device, function);
                if (base != 0 && mmioBaseOut != 0)
                    *mmioBaseOut = base;
                return 0;
            }
        }
    }
    return -1;
}

static void audioClearState(void)
{
    audioCtx.present = 0;
    audioCtx.initialized = 0;
    audioCtx.muted = 0;
    audioCtx.backend = AUDIO_BACKEND_NONE;
    audioCtx.hasPciAudio = 0;
    audioCtx.playing = 0;
    audioCtx.mmioBase = AUDIO_MMIO_DEFAULT_BASE;
    audioCtx.sampleRate = AUDIO_DEFAULT_SAMPLE_RATE;
    audioCtx.currentFrequency = 0;
    audioCtx.currentDivisor = 0;
    audioCtx.channels = AUDIO_DEFAULT_CHANNELS;
    audioCtx.bitsPerSample = AUDIO_DEFAULT_BITS;
    audioCtx.volume = AUDIO_DEFAULT_VOLUME;
}

int audioDetectHardware(uint32_t mmioBase)
{
    audioCtx.mmioBase = mmioBase;

    if (pciFindAudioDevice(&audioCtx.mmioBase) == 0)
        audioCtx.hasPciAudio = 1;
    else
        audioCtx.hasPciAudio = 0;

    audioCtx.present = 1;
    audioCtx.backend = AUDIO_BACKEND_PC_SPEAKER;
    return 0;
}

int audioResetHardware(void)
{
    if (!audioCtx.present)
        return -1;

    audioCtx.currentDivisor = 0;
    audioCtx.currentFrequency = 0;
    audioSilenceSpeaker();
    return 0;
}

int audioConfigureFormat(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample)
{
    if (!audioCtx.present)
        return -1;
    if (sampleRate == 0)
        return -2;
    if (channels != 1 && channels != 2)
        return -3;
    if (bitsPerSample != 8 && bitsPerSample != 16)
        return -4;

    audioCtx.sampleRate = sampleRate;
    audioCtx.channels = channels;
    audioCtx.bitsPerSample = bitsPerSample;

    return 0;
}

int audioEnableInterrupts(void)
{
    if (!audioCtx.present)
        return -1;

    return 0;
}

int audioPlayTone(uint32_t frequency)
{
    if (!audioCtx.present)
        return -1;
    if (!audioCtx.initialized)
        return -2;
    if (audioCtx.backend != AUDIO_BACKEND_PC_SPEAKER)
        return -3;

    if (frequency == 0)
        return audioStopTone();

    uint16_t divisor = audioFrequencyToDivisor(frequency);
    if (divisor == 0)
        return -4;

    audioCtx.currentDivisor = divisor;
    audioCtx.currentFrequency = PIT_BASE_FREQUENCY / divisor;
    audioApplyOutputState();
    return 0;
}

int audioStopTone(void)
{
    if (!audioCtx.present)
        return -1;

    audioCtx.currentDivisor = 0;
    audioCtx.currentFrequency = 0;
    audioApplyOutputState();
    return 0;
}

int audioInitDriver(void)
{
    audioClearState();

    if (audioDetectHardware(AUDIO_MMIO_DEFAULT_BASE) != 0)
        return -1;
    if (audioResetHardware() != 0)
        return -2;
    if (audioConfigureFormat(AUDIO_DEFAULT_SAMPLE_RATE, AUDIO_DEFAULT_CHANNELS, AUDIO_DEFAULT_BITS) != 0)
        return -3;
    if (audioEnableInterrupts() != 0)
        return -4;

    audioCtx.initialized = 1;
    return 0;
}

audio_driver_status_t audioGetStatus(void)
{
    audio_driver_status_t status = {
        .sampleRate = audioCtx.sampleRate,
        .currentFrequency = audioCtx.currentFrequency,
        .present = audioCtx.present,
        .initialized = audioCtx.initialized,
        .muted = audioCtx.muted,
        .playing = audioCtx.playing,
        .backend = audioCtx.backend,
        .hasPciAudio = audioCtx.hasPciAudio,
        .channels = audioCtx.channels,
        .bitsPerSample = audioCtx.bitsPerSample,
        .volume = audioCtx.volume};
    return status;
}

int audioMute(uint8_t enable)
{
    if (!audioCtx.present)
        return -1;

    audioCtx.muted = enable ? 1 : 0;
    audioApplyOutputState();
    return 0;
}
