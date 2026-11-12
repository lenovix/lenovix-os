#ifndef BASIC_H
#define BASIC_H

#include "terminal.h"

void basic_init(void);
int basic_is_active(void);
void basic_enter(terminal_t *term);
void basic_exit(terminal_t *term);
void basic_handle_input(terminal_t *term, const char *line);
void basic_run_file(terminal_t *term, const char *filename);

#endif
