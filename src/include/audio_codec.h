#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

#define AUDIO_FORMAT_UNKNOWN 0
#define AUDIO_FORMAT_WAV 1
#define AUDIO_FORMAT_MP3 2
#define AUDIO_FORMAT_CHIP 3

#define SPECTRUM_BANDS 16

typedef struct
{
    int format;
    char title[32];
    unsigned int sample_rate;
    int channels;
    int bits_per_sample;
    unsigned int total_samples;
    unsigned int current_sample;
    int is_playing;
    int is_paused;
    int volume;
    unsigned char spectrum[SPECTRUM_BANDS];
    unsigned char peaks[SPECTRUM_BANDS];
    unsigned char peak_delay[SPECTRUM_BANDS];
} audio_player_state_t;

void audio_init(void);
audio_player_state_t *audio_get_state(void);

int audio_load_file(const char *filename);
void audio_play(void);
void audio_pause(void);
void audio_stop(void);
void audio_toggle_play_pause(void);
void audio_set_volume(int vol);

void audio_update_stream(void);
void audio_calculate_spectrum(const unsigned char *pcm_data, unsigned int len);

int wav_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels);
int mp3_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels);
int chip_generate_pcm(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out);

#endif
