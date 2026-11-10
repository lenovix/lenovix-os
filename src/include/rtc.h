#ifndef RTC_H
#define RTC_H

typedef struct
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;
} rtc_time_t;

void rtc_init(void);
rtc_time_t rtc_get_time(void);
void rtc_get_time_str(char *buf);

#endif
