#ifndef MOUSE_H
#define MOUSE_H

typedef struct {
    int x;
    int y;
    unsigned char left_button;
    unsigned char right_button;
    unsigned char middle_button;
} mouse_state_t;

void mouse_init(void);
void mouse_handler(void);
mouse_state_t* mouse_get_state(void);
void vesa_draw_cursor(int x, int y);

#endif