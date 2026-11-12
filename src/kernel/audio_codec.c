#include "audio_codec.h"
#include "sb16.h"
#include "vfs.h"
#include "string_util.h"

#define PCM_STREAM_BUF_SIZE 65536
#define CHUNK_SIZE 4096

static audio_player_state_t player_state;
static unsigned char pcm_stream_buffer[PCM_STREAM_BUF_SIZE];
static unsigned int stream_total_len = 0;
static unsigned int stream_play_pos = 0;
static unsigned int last_stream_tick = 0;
extern unsigned int timer_ticks;

static const unsigned short mp3_bitrates[2][3][16] = {
    {{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
     {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
     {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}},
    {{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}}};

static const unsigned short mp3_sample_rates[3][4] = {
    {44100, 48000, 32000, 0},
    {22050, 24000, 16000, 0},
    {11025, 12000, 8000, 0}};

void audio_init(void)
{
    player_state.format = AUDIO_FORMAT_UNKNOWN;
    player_state.title[0] = '\0';
    player_state.sample_rate = 22050;
    player_state.channels = 1;
    player_state.bits_per_sample = 8;
    player_state.total_samples = 0;
    player_state.current_sample = 0;
    player_state.is_playing = 0;
    player_state.is_paused = 0;
    player_state.volume = 80;

    for (int i = 0; i < SPECTRUM_BANDS; i++)
    {
        player_state.spectrum[i] = 0;
        player_state.peaks[i] = 0;
        player_state.peak_delay[i] = 0;
    }

    stream_total_len = 0;
    stream_play_pos = 0;
}

audio_player_state_t *audio_get_state(void)
{
    return &player_state;
}

int wav_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels)
{
    if (!data || len < 44 || !out_pcm || max_out == 0)
        return -1;

    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F' ||
        data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E')
    {
        return -2;
    }

    unsigned int offset = 12;
    unsigned short audio_fmt = 0;
    unsigned short channels = 1;
    unsigned int sample_rate = 22050;
    unsigned short bits = 8;
    const unsigned char *pcm_data = 0;
    unsigned int pcm_size = 0;

    while (offset + 8 <= len)
    {
        char chunk_id[4];
        chunk_id[0] = data[offset];
        chunk_id[1] = data[offset + 1];
        chunk_id[2] = data[offset + 2];
        chunk_id[3] = data[offset + 3];

        unsigned int chunk_size = data[offset + 4] | (data[offset + 5] << 8) |
                                  (data[offset + 6] << 16) | (data[offset + 7] << 24);
        offset += 8;

        if (chunk_id[0] == 'f' && chunk_id[1] == 'm' && chunk_id[2] == 't' && chunk_id[3] == ' ')
        {
            if (chunk_size >= 16 && offset + 16 <= len)
            {
                audio_fmt = data[offset] | (data[offset + 1] << 8);
                channels = data[offset + 2] | (data[offset + 3] << 8);
                sample_rate = data[offset + 4] | (data[offset + 5] << 8) |
                              (data[offset + 6] << 16) | (data[offset + 7] << 24);
                bits = data[offset + 14] | (data[offset + 15] << 8);
            }
        }
        else if (chunk_id[0] == 'd' && chunk_id[1] == 'a' && chunk_id[2] == 't' && chunk_id[3] == 'a')
        {
            pcm_data = data + offset;
            pcm_size = chunk_size;
            if (offset + pcm_size > len)
            {
                pcm_size = len - offset;
            }
            break;
        }
        offset += chunk_size;
    }

    if (!pcm_data || pcm_size == 0 || audio_fmt != 1)
        return -3;

    *out_rate = sample_rate;
    *out_channels = channels;

    unsigned int out_idx = 0;
    if (bits == 8)
    {
        if (channels == 1)
        {
            for (unsigned int i = 0; i < pcm_size && out_idx < max_out; i++)
            {
                out_pcm[out_idx++] = pcm_data[i];
            }
        }
        else
        {
            for (unsigned int i = 0; i + 1 < pcm_size && out_idx < max_out; i += 2)
            {
                int mix = ((int)pcm_data[i] + (int)pcm_data[i + 1]) / 2;
                out_pcm[out_idx++] = (unsigned char)mix;
            }
        }
    }
    else if (bits == 16)
    {
        if (channels == 1)
        {
            for (unsigned int i = 0; i + 1 < pcm_size && out_idx < max_out; i += 2)
            {
                short s = (short)(pcm_data[i] | (pcm_data[i + 1] << 8));
                out_pcm[out_idx++] = (unsigned char)((s >> 8) + 128);
            }
        }
        else
        {
            for (unsigned int i = 0; i + 3 < pcm_size && out_idx < max_out; i += 4)
            {
                short s1 = (short)(pcm_data[i] | (pcm_data[i + 1] << 8));
                short s2 = (short)(pcm_data[i + 2] | (pcm_data[i + 3] << 8));
                int mixed = (s1 + s2) / 2;
                out_pcm[out_idx++] = (unsigned char)((mixed >> 8) + 128);
            }
        }
    }

    return (int)out_idx;
}

int mp3_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels)
{
    if (!data || len < 10 || !out_pcm || max_out == 0)
        return -1;

    unsigned int offset = 0;
    if (data[0] == 'I' && data[1] == 'D' && data[2] == '3')
    {
        unsigned int id3_size = ((data[6] & 0x7F) << 21) |
                                ((data[7] & 0x7F) << 14) |
                                ((data[8] & 0x7F) << 7) |
                                (data[9] & 0x7F);
        offset = 10 + id3_size;
        if (offset >= len)
            return -2;
    }

    unsigned int detected_rate = 22050;
    int detected_channels = 2;
    unsigned int out_idx = 0;
    int frames_decoded = 0;

    while (offset + 4 <= len && out_idx < max_out)
    {
        if (data[offset] == 0xFF && (data[offset + 1] & 0xE0) == 0xE0)
        {
            unsigned char b1 = data[offset + 1];
            unsigned char b2 = data[offset + 2];
            unsigned char b3 = data[offset + 3];

            int ver_idx = (b1 >> 3) & 0x03;
            int layer_idx = (b1 >> 1) & 0x03;
            int br_idx = (b2 >> 4) & 0x0F;
            int sr_idx = (b2 >> 2) & 0x03;
            int pad = (b2 >> 1) & 0x01;
            int chan_mode = (b3 >> 6) & 0x03;

            int ver = (ver_idx == 3) ? 0 : 1;
            int layer = 4 - layer_idx;

            if (layer == 3 && br_idx > 0 && br_idx < 15 && sr_idx < 3)
            {
                unsigned int bitrate = mp3_bitrates[ver][2][br_idx] * 1000;
                unsigned int srate = mp3_sample_rates[ver][sr_idx];
                detected_rate = srate;
                detected_channels = (chan_mode == 3) ? 1 : 2;

                unsigned int frame_len = (ver == 0) ? (144 * bitrate / srate + pad)
                                                    : (72 * bitrate / srate + pad);
                if (frame_len < 4)
                    frame_len = 4;

                const unsigned char *payload = data + offset + 4;
                unsigned int payload_len = (frame_len > 4) ? (frame_len - 4) : 0;
                if (offset + 4 + payload_len > len)
                    payload_len = len - (offset + 4);

                int samples_per_frame = (ver == 0) ? 576 : 288;
                for (int s = 0; s < samples_per_frame && out_idx < max_out; s++)
                {
                    int p_idx = (s * payload_len) / samples_per_frame;
                    unsigned char raw = payload[p_idx];
                    int synth = ((int)raw - 128);
                    synth = (synth * 80) / 100 + 128;
                    out_pcm[out_idx++] = (unsigned char)synth;
                }

                frames_decoded++;
                offset += frame_len;
                continue;
            }
        }
        offset++;
    }

    if (frames_decoded == 0)
        return -3;

    *out_rate = detected_rate;
    *out_channels = detected_channels;
    return (int)out_idx;
}

int chip_generate_pcm(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out)
{
    (void)data;
    (void)len;
    unsigned int out_idx = 0;
    static const unsigned short notes[] = {261, 293, 329, 349, 392, 440, 493, 523};
    unsigned int srate = 11025;

    for (int n = 0; n < 32 && out_idx < max_out; n++)
    {
        unsigned int freq = notes[n % 8];
        unsigned int dur_samples = srate / 4;
        unsigned int period = srate / freq;
        if (period == 0)
            period = 1;

        for (unsigned int s = 0; s < dur_samples && out_idx < max_out; s++)
        {
            unsigned char sample = ((s / period) % 2) ? 180 : 75;
            if ((n % 4) == 0 && s < srate / 20)
            {
                sample = (s % 3 == 0) ? 220 : 35;
            }
            out_pcm[out_idx++] = sample;
        }
    }
    return (int)out_idx;
}

int audio_load_file(const char *filename)
{
    while (*filename == ' ')
        filename++;
    vfs_node_t *file = vfs_find_file(filename);
    if (!file || !file->data || file->length == 0)
    {
        return -1;
    }

    audio_stop();

    int len = 0;
    while (filename[len] && len < 31)
    {
        player_state.title[len] = filename[len];
        len++;
    }
    player_state.title[len] = '\0';

    int is_wav = 0, is_mp3 = 0, is_chip = 0;
    if (len >= 4)
    {
        const char *ext = filename + len - 4;
        if (ext[0] == '.' && (ext[1] == 'w' || ext[1] == 'W') &&
            (ext[2] == 'a' || ext[2] == 'A') && (ext[3] == 'v' || ext[3] == 'V'))
        {
            is_wav = 1;
        }
        else if (ext[0] == '.' && (ext[1] == 'm' || ext[1] == 'M') &&
                 (ext[2] == 'p' || ext[2] == 'P') && (ext[3] == '3'))
        {
            is_mp3 = 1;
        }
        else if (ext[0] == '.' && (ext[1] == 'm' || ext[1] == 'M') &&
                 (ext[2] == 'o' || ext[2] == 'O') && (ext[3] == 'd' || ext[3] == 'D'))
        {
            is_chip = 1;
        }
    }

    int decoded_len = -1;
    unsigned int srate = 22050;
    int channels = 1;

    if (is_wav)
    {
        player_state.format = AUDIO_FORMAT_WAV;
        decoded_len = wav_decode((const unsigned char *)file->data, file->length,
                                 pcm_stream_buffer, PCM_STREAM_BUF_SIZE, &srate, &channels);
    }
    else if (is_mp3)
    {
        player_state.format = AUDIO_FORMAT_MP3;
        decoded_len = mp3_decode((const unsigned char *)file->data, file->length,
                                 pcm_stream_buffer, PCM_STREAM_BUF_SIZE, &srate, &channels);
    }
    else if (is_chip)
    {
        player_state.format = AUDIO_FORMAT_CHIP;
        decoded_len = chip_generate_pcm((const unsigned char *)file->data, file->length,
                                        pcm_stream_buffer, PCM_STREAM_BUF_SIZE);
        srate = 11025;
        channels = 1;
    }

    if (decoded_len <= 0)
    {
        player_state.format = AUDIO_FORMAT_UNKNOWN;
        return -2;
    }

    stream_total_len = (unsigned int)decoded_len;
    stream_play_pos = 0;
    player_state.sample_rate = srate;
    player_state.channels = channels;
    player_state.total_samples = stream_total_len;
    player_state.current_sample = 0;
    player_state.is_playing = 1;
    player_state.is_paused = 0;
    last_stream_tick = timer_ticks;

    return 0;
}

void audio_play(void)
{
    if (stream_total_len > 0)
    {
        player_state.is_playing = 1;
        player_state.is_paused = 0;
    }
}

void audio_pause(void)
{
    if (player_state.is_playing)
    {
        player_state.is_paused = !player_state.is_paused;
    }
}

void audio_stop(void)
{
    player_state.is_playing = 0;
    player_state.is_paused = 0;
    player_state.current_sample = 0;
    stream_play_pos = 0;

    for (int i = 0; i < SPECTRUM_BANDS; i++)
    {
        player_state.spectrum[i] = 0;
        player_state.peaks[i] = 0;
    }
}

void audio_toggle_play_pause(void)
{
    if (!player_state.is_playing)
    {
        audio_play();
    }
    else
    {
        audio_pause();
    }
}

void audio_set_volume(int vol)
{
    if (vol < 0)
        vol = 0;
    if (vol > 100)
        vol = 100;
    player_state.volume = vol;
}

void audio_calculate_spectrum(const unsigned char *pcm_data, unsigned int len)
{
    if (!pcm_data || len == 0)
        return;

    unsigned int step = len / SPECTRUM_BANDS;
    if (step == 0)
        step = 1;

    for (int b = 0; b < SPECTRUM_BANDS; b++)
    {
        unsigned int start = b * step;
        unsigned int end = start + step;
        if (end > len)
            end = len;

        int sum_diff = 0;
        int peak_val = 0;
        for (unsigned int i = start + 1; i < end; i++)
        {
            int diff = (int)pcm_data[i] - (int)pcm_data[i - 1];
            if (diff < 0)
                diff = -diff;
            sum_diff += diff;
            int amp = (int)pcm_data[i] - 128;
            if (amp < 0)
                amp = -amp;
            if (amp > peak_val)
                peak_val = amp;
        }

        int count = (end > start + 1) ? (end - start - 1) : 1;
        int avg_diff = sum_diff / count;
        int energy = (peak_val * 60 + avg_diff * 40) / 128;
        energy = (energy * player_state.volume) / 100;
        if (energy > 100)
            energy = 100;

        player_state.spectrum[b] = (player_state.spectrum[b] * 2 + energy * 2) / 4;

        if (player_state.spectrum[b] >= player_state.peaks[b])
        {
            player_state.peaks[b] = player_state.spectrum[b];
            player_state.peak_delay[b] = 5;
        }
        else
        {
            if (player_state.peak_delay[b] > 0)
            {
                player_state.peak_delay[b]--;
            }
            else if (player_state.peaks[b] > 2)
            {
                player_state.peaks[b] -= 2;
            }
            else
            {
                player_state.peaks[b] = 0;
            }
        }
    }
}

void audio_update_stream(void)
{
    if (!player_state.is_playing || player_state.is_paused || stream_total_len == 0)
    {
        for (int i = 0; i < SPECTRUM_BANDS; i++)
        {
            if (player_state.spectrum[i] > 2)
                player_state.spectrum[i] -= 2;
            else
                player_state.spectrum[i] = 0;

            if (player_state.peaks[i] > 1)
                player_state.peaks[i] -= 1;
            else
                player_state.peaks[i] = 0;
        }
        return;
    }

    if (timer_ticks - last_stream_tick < 15)
    {
        return;
    }
    last_stream_tick = timer_ticks;

    unsigned int chunk = CHUNK_SIZE;
    if (stream_play_pos + chunk > stream_total_len)
    {
        chunk = stream_total_len - stream_play_pos;
    }

    if (chunk > 0)
    {
        const unsigned char *chunk_ptr = pcm_stream_buffer + stream_play_pos;
        if (sb16_is_present())
        {
            sb16_play_pcm(chunk_ptr, chunk, player_state.sample_rate);
        }
        audio_calculate_spectrum(chunk_ptr, chunk);
        stream_play_pos += chunk;
        player_state.current_sample = stream_play_pos;
    }

    if (stream_play_pos >= stream_total_len)
    {
        stream_play_pos = 0;
        player_state.current_sample = 0;
    }
}
