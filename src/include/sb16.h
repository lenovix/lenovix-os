#ifndef SB16_H
#define SB16_H

typedef struct
{
    char riff_id[4]; // "RIFF"
    unsigned int riff_size;
    char wave_id[4]; // "WAVE"
    char fmt_id[4];  // "fmt "
    unsigned int fmt_size;
    unsigned short audio_format; // 1 = PCM
    unsigned short num_channels; // 1 = Mono, 2 = Stereo
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample; // 8 or 16
    char data_id[4];                // "data"
    unsigned int data_size;
} __attribute__((packed)) wav_header_t;

int sb16_init(void);
int sb16_is_present(void);
int sb16_play_pcm(const unsigned char *data, unsigned int len, unsigned int sample_rate);
int sb16_play_wav(const void *data, unsigned int len);
void sb16_play_sample_sound(void);

#endif
