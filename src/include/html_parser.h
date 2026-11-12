#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#define HTML_MAX_LINES 256
#define HTML_LINE_LEN 80

typedef struct
{
    char text[HTML_LINE_LEN];
    unsigned int color;
    int is_heading;
    int is_link;
    char href[64];
} html_line_t;

typedef struct
{
    char title[64];
    html_line_t lines[HTML_MAX_LINES];
    int line_count;
} html_doc_t;

void html_parse(const char *html_raw, html_doc_t *doc, int max_chars_per_line);

#endif
