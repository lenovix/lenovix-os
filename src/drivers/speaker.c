#include "speaker.h"
#include "io_util.h"

extern void sleep(unsigned int ticks);

static void delay_short(unsigned int count)
{
    for (volatile unsigned int i = 0; i < count * 5000; i++)
    {
    }
}

void speaker_play(unsigned int frequency)
{
    if (frequency == 0)
    {
        speaker_stop();
        return;
    }

    unsigned int divisor = 1193180 / frequency;

    outb(0x43, 0xB6);
    outb(0x42, (unsigned char)(divisor & 0xFF));
    outb(0x42, (unsigned char)((divisor >> 8) & 0xFF));

    unsigned char tmp = inb(0x61);
    if ((tmp & 3) != 3)
    {
        outb(0x61, tmp | 3);
    }
}

void speaker_stop(void)
{
    unsigned char tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

void speaker_beep(unsigned int frequency, unsigned int duration_ms)
{
    speaker_play(frequency);
    unsigned int ticks = (duration_ms + 9) / 10;
    if (ticks == 0)
        ticks = 1;
    sleep(ticks);
    speaker_stop();
}

void speaker_click(void)
{
    speaker_play(800);
    delay_short(2);
    speaker_stop();
}

void speaker_play_chime(void)
{
    speaker_beep(523, 70);
    speaker_beep(659, 70);
    speaker_beep(784, 90);
    speaker_beep(1046, 140);
}
