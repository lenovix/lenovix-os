#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SPECTRUM_BANDS 16

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

int test_wav_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels)
{
    if (!data || len < 44 || !out_pcm || max_out == 0)
        return -1;
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F' ||
        data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E')
        return -2;

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
                pcm_size = len - offset;
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
        for (unsigned int i = 0; i < pcm_size && out_idx < max_out; i++)
        {
            out_pcm[out_idx++] = pcm_data[i];
        }
    }
    return (int)out_idx;
}

int test_mp3_decode(const unsigned char *data, unsigned int len, unsigned char *out_pcm, unsigned int max_out, unsigned int *out_rate, int *out_channels)
{
    unsigned int offset = 0;
    unsigned int detected_rate = 22050;
    int detected_channels = 2;
    unsigned int out_idx = 0;
    int frames = 0;

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

                int samples_per_frame = 288;
                for (int s = 0; s < samples_per_frame && out_idx < max_out; s++)
                {
                    int p_idx = (s * payload_len) / samples_per_frame;
                    out_pcm[out_idx++] = payload[p_idx];
                }
                frames++;
                offset += frame_len;
                continue;
            }
        }
        offset++;
    }
    if (frames == 0)
        return -3;
    *out_rate = detected_rate;
    *out_channels = detected_channels;
    return (int)out_idx;
}

void test_spectrum(const unsigned char *pcm, unsigned int len, unsigned char *spectrum)
{
    unsigned int step = len / SPECTRUM_BANDS;
    if (step == 0)
        step = 1;
    for (int b = 0; b < SPECTRUM_BANDS; b++)
    {
        unsigned int start = b * step;
        unsigned int end = start + step;
        if (end > len)
            end = len;
        int peak = 0;
        for (unsigned int i = start; i < end; i++)
        {
            int amp = (int)pcm[i] - 128;
            if (amp < 0)
                amp = -amp;
            if (amp > peak)
                peak = amp;
        }
        int energy = (peak * 100) / 128;
        if (energy > 100)
            energy = 100;
        spectrum[b] = (unsigned char)energy;
    }
}

int main(void)
{
    // 1. Test WAV
    unsigned char wav[128];
    memset(wav, 0, sizeof(wav));
    memcpy(wav, "RIFF", 4);
    unsigned int rlen = 120;
    memcpy(wav + 4, &rlen, 4);
    memcpy(wav + 8, "WAVEfmt ", 8);
    unsigned int fsize = 16;
    memcpy(wav + 16, &fsize, 4);
    unsigned short afmt = 1, ch = 1, bits = 8;
    unsigned int sr = 11025;
    memcpy(wav + 20, &afmt, 2);
    memcpy(wav + 22, &ch, 2);
    memcpy(wav + 24, &sr, 4);
    memcpy(wav + 28, &sr, 4);
    unsigned short align = 1;
    memcpy(wav + 32, &align, 2);
    memcpy(wav + 34, &bits, 2);
    memcpy(wav + 36, "data", 4);
    unsigned int dlen = 80;
    memcpy(wav + 40, &dlen, 4);
    for (int i = 0; i < 80; i++)
        wav[44 + i] = (i % 2) ? 200 : 50;

    unsigned char out_pcm[2048];
    unsigned int out_rate = 0;
    int out_ch = 0;
    int res = test_wav_decode(wav, 128, out_pcm, 2048, &out_rate, &out_ch);
    assert(res == 80);
    assert(out_rate == 11025);
    assert(out_ch == 1);

    // 2. Test MP3
    unsigned char mp3[500];
    mp3[0] = 0xFF;
    mp3[1] = 0xFB; // MPEG-1 Layer 3
    mp3[2] = 0x90; // 128kbps, 44100Hz
    mp3[3] = 0x64; // Stereo
    for (int i = 4; i < 417; i++)
        mp3[i] = (unsigned char)(i * 3);
    res = test_mp3_decode(mp3, 417, out_pcm, 2048, &out_rate, &out_ch);
    assert(res == 288);
    assert(out_rate == 44100);
    assert(out_ch == 2);

    // 3. Test Spectrum calculation
    unsigned char spectrum[SPECTRUM_BANDS];
    test_spectrum(out_pcm, res, spectrum);
    int active_bands = 0;
    for (int b = 0; b < SPECTRUM_BANDS; b++)
    {
        assert(spectrum[b] <= 100);
        if (spectrum[b] > 0)
            active_bands++;
    }
    assert(active_bands > 0);

    printf("ALL AUDIO & SPECTRUM TESTS PASSED\n");
    return 0;
}
