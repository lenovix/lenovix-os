#include "sb16.h"
#include "io_util.h"

#define SB16_BASE 0x220

#define SB_MIXER_REG 0x224
#define SB_MIXER_DAT 0x225
#define SB_DSP_RESET 0x226
#define SB_DSP_READ 0x22A
#define SB_DSP_WRITE 0x22C
#define SB_DSP_STATUS 0x22E

static int sb16_detected = 0;
static unsigned char dma_pcm_buffer[8192] __attribute__((aligned(4096)));

static void sb_delay(void)
{
    for (volatile int i = 0; i < 2000; i++)
    {
    }
}

static int dsp_reset(void)
{
    outb(SB_DSP_RESET, 1);
    sb_delay();
    outb(SB_DSP_RESET, 0);

    for (int i = 0; i < 1000; i++)
    {
        if (inb(SB_DSP_STATUS) & 0x80)
        {
            if (inb(SB_DSP_READ) == 0xAA)
            {
                return 0;
            }
        }
        sb_delay();
    }
    return -1;
}

static void dsp_write(unsigned char val)
{
    int timeout = 50000;
    while ((inb(SB_DSP_WRITE) & 0x80) && --timeout)
    {
    }
    outb(SB_DSP_WRITE, val);
}

int sb16_init(void)
{
    if (dsp_reset() != 0)
    {
        sb16_detected = 0;
        return -1;
    }

    outb(SB_MIXER_REG, 0x22);
    outb(SB_MIXER_DAT, 0xFF);
    outb(SB_MIXER_REG, 0x04);
    outb(SB_MIXER_DAT, 0xFF);

    sb16_detected = 1;
    return 0;
}

int sb16_is_present(void)
{
    return sb16_detected;
}

static void dma_setup_channel1(unsigned int phys_addr, unsigned int count)
{
    outb(0x0A, 0x05);
    outb(0x0C, 0x00);
    outb(0x0B, 0x49);

    outb(0x02, (unsigned char)(phys_addr & 0xFF));
    outb(0x02, (unsigned char)((phys_addr >> 8) & 0xFF));
    outb(0x83, (unsigned char)((phys_addr >> 16) & 0xFF));

    unsigned int cnt = count - 1;
    outb(0x03, (unsigned char)(cnt & 0xFF));
    outb(0x03, (unsigned char)((cnt >> 8) & 0xFF));

    outb(0x0A, 0x01);
}

int sb16_play_pcm(const unsigned char *data, unsigned int len, unsigned int sample_rate)
{
    if (!sb16_detected || !data || len == 0)
        return -1;

    if (len > sizeof(dma_pcm_buffer))
    {
        len = sizeof(dma_pcm_buffer);
    }

    for (unsigned int i = 0; i < len; i++)
    {
        dma_pcm_buffer[i] = data[i];
    }

    dma_setup_channel1((unsigned int)dma_pcm_buffer, len);

    dsp_write(0x41);
    dsp_write((unsigned char)((sample_rate >> 8) & 0xFF));
    dsp_write((unsigned char)(sample_rate & 0xFF));

    dsp_write(0xD1);

    dsp_write(0x14);
    unsigned int cnt = len - 1;
    dsp_write((unsigned char)(cnt & 0xFF));
    dsp_write((unsigned char)((cnt >> 8) & 0xFF));

    return 0;
}

int sb16_play_wav(const void *data, unsigned int len)
{
    if (!sb16_detected || !data || len < sizeof(wav_header_t))
        return -1;

    const wav_header_t *wav = (const wav_header_t *)data;
    if (wav->riff_id[0] != 'R' || wav->riff_id[1] != 'I' ||
        wav->riff_id[2] != 'F' || wav->riff_id[3] != 'F' ||
        wav->wave_id[0] != 'W' || wav->wave_id[1] != 'A' ||
        wav->wave_id[2] != 'V' || wav->wave_id[3] != 'E')
    {
        return -2;
    }

    if (wav->audio_format != 1 || wav->bits_per_sample != 8)
    {
        return -3;
    }

    const unsigned char *pcm = (const unsigned char *)data + sizeof(wav_header_t);
    unsigned int pcm_len = wav->data_size;
    if (pcm_len > len - sizeof(wav_header_t))
    {
        pcm_len = len - sizeof(wav_header_t);
    }

    return sb16_play_pcm(pcm, pcm_len, wav->sample_rate);
}

void sb16_play_sample_sound(void)
{
    static unsigned char sample[4000];
    for (int i = 0; i < 4000; i++)
    {
        int period = 20 - (i / 300);
        if (period < 8)
            period = 8;
        sample[i] = ((i / period) % 2) ? 200 : 55;
    }
    sb16_play_pcm(sample, 4000, 8000);
}
