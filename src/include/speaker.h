#ifndef SPEAKER_H
#define SPEAKER_H

void speaker_play(unsigned int frequency);
void speaker_stop(void);
void speaker_beep(unsigned int frequency, unsigned int duration_ms);
void speaker_click(void);
void speaker_play_chime(void);

#endif
