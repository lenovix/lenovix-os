#ifndef SHELL_H
#define SHELL_H

void process_command(void);
void handle_history_up(void);
void handle_history_down(void);

extern char command_buffer[256];
extern int command_index;

#endif