#include "html_parser.h"
#include "string_util.h"

static char to_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

static int str_case_eq(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        if (to_lower(*s1) != to_lower(*s2))
            return 0;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

static int str_case_starts(const char *s, const char *prefix)
{
    while (*prefix)
    {
        if (to_lower(*s) != to_lower(*prefix))
            return 0;
        s++;
        prefix++;
    }
    return 1;
}

static void doc_add_line(html_doc_t *doc, const char *txt, unsigned int color, int is_head, int is_lnk, const char *href)
{
    if (!doc || doc->line_count >= HTML_MAX_LINES)
        return;

    html_line_t *ln = &doc->lines[doc->line_count];
    int i = 0;
    while (txt[i] && i < HTML_LINE_LEN - 1)
    {
        ln->text[i] = txt[i];
        i++;
    }
    ln->text[i] = '\0';
    ln->color = color;
    ln->is_heading = is_head;
    ln->is_link = is_lnk;

    ln->href[0] = '\0';
    if (href && is_lnk)
    {
        int h = 0;
        while (href[h] && h < 63)
        {
            ln->href[h] = href[h];
            h++;
        }
        ln->href[h] = '\0';
    }

    doc->line_count++;
}

void html_parse(const char *html_raw, html_doc_t *doc, int max_chars_per_line)
{
    if (!html_raw || !doc)
        return;

    doc->line_count = 0;
    doc->title[0] = '\0';
    string_copy(doc->title, "Lenovix Browser");

    if (max_chars_per_line <= 10 || max_chars_per_line >= HTML_LINE_LEN)
    {
        max_chars_per_line = 50;
    }

    const char *p = html_raw;
    char cur_line[HTML_LINE_LEN];
    int line_len = 0;
    unsigned int cur_color = 0xFFFFFF;
    int in_heading = 0;
    int in_link = 0;
    char cur_href[64] = {0};

    while (*p && doc->line_count < HTML_MAX_LINES)
    {
        // Skip comments
        if (p[0] == '<' && p[1] == '!' && p[2] == '-' && p[3] == '-')
        {
            p += 4;
            while (*p && !(p[0] == '-' && p[1] == '-' && p[2] == '>'))
                p++;
            if (*p)
                p += 3;
            continue;
        }

        // HTML Tags
        if (*p == '<')
        {
            p++;
            char tag[32];
            int ti = 0;
            while (*p && *p != '>' && *p != ' ' && *p != '\t' && *p != '\n' && ti < 31)
            {
                tag[ti++] = *p++;
            }
            tag[ti] = '\0';

            char attr[128] = {0};
            int ai = 0;
            while (*p && *p != '>')
            {
                if (ai < 127)
                    attr[ai++] = *p;
                p++;
            }
            attr[ai] = '\0';
            if (*p == '>')
                p++;

            // Handle title
            if (str_case_eq(tag, "title"))
            {
                int tlen = 0;
                while (*p && *p != '<' && tlen < 63)
                {
                    if (*p != '\r' && *p != '\n')
                    {
                        doc->title[tlen++] = *p;
                    }
                    p++;
                }
                doc->title[tlen] = '\0';
                continue;
            }

            // Skip script & style
            if (str_case_eq(tag, "script") || str_case_eq(tag, "style"))
            {
                char close_tag[32];
                close_tag[0] = '/';
                int k = 0;
                while (tag[k] && k < 30)
                {
                    close_tag[k + 1] = tag[k];
                    k++;
                }
                close_tag[k + 1] = '\0';
                while (*p)
                {
                    if (*p == '<' && str_case_starts(p + 1, close_tag))
                    {
                        while (*p && *p != '>')
                            p++;
                        if (*p == '>')
                            p++;
                        break;
                    }
                    p++;
                }
                continue;
            }

            // Headings h1, h2, h3
            if (str_case_eq(tag, "h1") || str_case_eq(tag, "h2") || str_case_eq(tag, "h3"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                in_heading = 1;
                cur_color = 0x00D2FF;
                continue;
            }
            if (str_case_eq(tag, "/h1") || str_case_eq(tag, "/h2") || str_case_eq(tag, "/h3"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                in_heading = 0;
                cur_color = 0xFFFFFF;
                doc_add_line(doc, "", 0xFFFFFF, 0, 0, NULL);
                continue;
            }

            // Paragraphs and breaks
            if (str_case_eq(tag, "p") || str_case_eq(tag, "div"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                continue;
            }
            if (str_case_eq(tag, "/p") || str_case_eq(tag, "/div"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                doc_add_line(doc, "", 0xFFFFFF, 0, 0, NULL);
                continue;
            }
            if (str_case_eq(tag, "br"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                else
                {
                    doc_add_line(doc, "", 0xFFFFFF, 0, 0, NULL);
                }
                continue;
            }
            if (str_case_eq(tag, "hr"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                doc_add_line(doc, "--------------------------------------------------", 0x888888, 0, 0, NULL);
                continue;
            }

            // Lists
            if (str_case_eq(tag, "li"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                cur_line[line_len++] = '*';
                cur_line[line_len++] = ' ';
                continue;
            }
            if (str_case_eq(tag, "/li"))
            {
                if (line_len > 0)
                {
                    cur_line[line_len] = '\0';
                    doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
                    line_len = 0;
                }
                continue;
            }

            // Links <a href="...">
            if (str_case_eq(tag, "a"))
            {
                in_link = 1;
                cur_color = 0x2ECC71;
                cur_href[0] = '\0';
                const char *hp = attr;
                while (*hp)
                {
                    if (str_case_starts(hp, "href="))
                    {
                        hp += 5;
                        if (*hp == '"' || *hp == '\'')
                            hp++;
                        int hi = 0;
                        while (*hp && *hp != '"' && *hp != '\'' && *hp != ' ' && hi < 63)
                        {
                            cur_href[hi++] = *hp++;
                        }
                        cur_href[hi] = '\0';
                        break;
                    }
                    hp++;
                }
                continue;
            }
            if (str_case_eq(tag, "/a"))
            {
                in_link = 0;
                cur_color = in_heading ? 0x00D2FF : 0xFFFFFF;
                cur_href[0] = '\0';
                continue;
            }

            continue;
        }

        // HTML Entities
        char ch = *p;
        if (ch == '&')
        {
            if (str_case_starts(p, "&nbsp;"))
            {
                ch = ' ';
                p += 5;
            }
            else if (str_case_starts(p, "&amp;"))
            {
                ch = '&';
                p += 4;
            }
            else if (str_case_starts(p, "&lt;"))
            {
                ch = '<';
                p += 3;
            }
            else if (str_case_starts(p, "&gt;"))
            {
                ch = '>';
                p += 3;
            }
            else if (str_case_starts(p, "&quot;"))
            {
                ch = '"';
                p += 5;
            }
        }

        if (ch == '\r' || ch == '\n' || ch == '\t')
        {
            ch = ' ';
        }

        // Avoid multiple consecutive spaces
        if (ch == ' ' && line_len > 0 && cur_line[line_len - 1] == ' ')
        {
            p++;
            continue;
        }

        cur_line[line_len++] = ch;
        p++;

        if (line_len >= max_chars_per_line)
        {
            int break_pt = line_len;
            while (break_pt > max_chars_per_line / 2 && cur_line[break_pt - 1] != ' ')
            {
                break_pt--;
            }
            if (break_pt <= max_chars_per_line / 2)
            {
                break_pt = max_chars_per_line;
            }
            char save_overflow[HTML_LINE_LEN];
            int ov_len = 0;
            for (int k = break_pt; k < line_len; k++)
            {
                if (k == break_pt && cur_line[k] == ' ')
                    continue;
                save_overflow[ov_len++] = cur_line[k];
            }
            cur_line[break_pt] = '\0';
            doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);

            for (int k = 0; k < ov_len; k++)
                cur_line[k] = save_overflow[k];
            line_len = ov_len;
        }
    }

    if (line_len > 0)
    {
        cur_line[line_len] = '\0';
        doc_add_line(doc, cur_line, cur_color, in_heading, in_link, cur_href);
    }
}
