#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#ifndef NULL
#define NULL ((void *)0)
#endif

void string_copy(char *dest, const char *src);
int string_equals(const char *str1, const char *str2);
int string_length(const char *str);

#endif
