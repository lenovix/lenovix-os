#include <assert.h>
#include <stdio.h>
#include <string.h>

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

extern void html_parse(const char *html_raw, html_doc_t *doc, int max_chars_per_line);

void string_copy(char *dest, const char *src)
{
    int i = 0;
    while (src[i] && i < 255)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}
int string_length(const char *s)
{
    int len = 0;
    while (s && s[len])
        len++;
    return len;
}

int main(void)
{
    html_doc_t doc;
    const char *html =
        "<html><head><title>Test Page</title><style>body { color: red; }</style></head><body>"
        "<h1>Welcome</h1>"
        "<p>This is a test paragraph with a <a href=\"next.html\">click here</a> link.</p>"
        "<hr>"
        "<ul><li>First Item</li><li>Second Item</li></ul>"
        "</body></html>";

    html_parse(html, &doc, 40);

    assert(strcmp(doc.title, "Test Page") == 0);
    assert(doc.line_count > 0);

    int found_heading = 0;
    int found_link = 0;
    for (int i = 0; i < doc.line_count; i++)
    {
        if (doc.lines[i].is_heading && strstr(doc.lines[i].text, "Welcome"))
        {
            found_heading = 1;
        }
        if (doc.lines[i].is_link && strcmp(doc.lines[i].href, "next.html") == 0)
        {
            found_link = 1;
        }
    }
    assert(found_heading == 1);
    assert(found_link == 1);

    printf("ALL BROWSER HTML TESTS PASSED\n");
    return 0;
}
