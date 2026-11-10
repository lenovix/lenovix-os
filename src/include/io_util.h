#ifndef IO_UTIL_H
#define IO_UTIL_H

extern unsigned char inb(unsigned short port);

extern void outb(unsigned short port, unsigned char val);

static inline void outw(unsigned short port, unsigned short val)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

#endif
