#include <assert.h>
#include <stdio.h>

#define BASIC_MAX_LINES 128
#define BASIC_MAX_LINE_LEN 80

#define BASIC_STATUS_OK 0
#define BASIC_STATUS_END 1
#define BASIC_STATUS_ERROR 2

static int vars[26];
static int next_pc = -1;

typedef struct
{
    int num;
    char text[BASIC_MAX_LINE_LEN];
} basic_line_t;

static basic_line_t prog_lines[BASIC_MAX_LINES];
static int prog_line_count = 0;

static void skip_spaces(const char **p)
{
    while (**p == ' ' || **p == '\t')
        (*p)++;
}

static char to_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

static int match_keyword(const char **p, const char *kw)
{
    const char *orig = *p;
    while (*kw)
    {
        if (to_upper(**p) != to_upper(*kw))
        {
            *p = orig;
            return 0;
        }
        (*p)++;
        kw++;
    }
    char next = to_upper(**p);
    if ((next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
    {
        *p = orig;
        return 0;
    }
    return 1;
}

static int parse_int(const char **p)
{
    skip_spaces(p);
    int sign = 1;
    if (**p == '-')
    {
        sign = -1;
        (*p)++;
    }
    else if (**p == '+')
    {
        (*p)++;
    }
    int val = 0;
    while (**p >= '0' && **p <= '9')
    {
        val = val * 10 + (**p - '0');
        (*p)++;
    }
    return val * sign;
}

static int eval_expr(const char **p, int *err);

static int eval_factor(const char **p, int *err)
{
    skip_spaces(p);
    if (**p == '(')
    {
        (*p)++;
        int val = eval_expr(p, err);
        skip_spaces(p);
        if (**p == ')')
        {
            (*p)++;
        }
        else
        {
            *err = 1;
        }
        return val;
    }
    if (**p == '-')
    {
        (*p)++;
        return -eval_factor(p, err);
    }
    if (**p == '+')
    {
        (*p)++;
        return eval_factor(p, err);
    }
    if (**p >= '0' && **p <= '9')
    {
        int val = 0;
        while (**p >= '0' && **p <= '9')
        {
            val = val * 10 + (**p - '0');
            (*p)++;
        }
        return val;
    }
    char c = to_upper(**p);
    if (c >= 'A' && c <= 'Z')
    {
        (*p)++;
        return vars[c - 'A'];
    }
    *err = 1;
    return 0;
}

static int eval_term(const char **p, int *err)
{
    int left = eval_factor(p, err);
    while (1)
    {
        skip_spaces(p);
        if (**p == '*')
        {
            (*p)++;
            int right = eval_factor(p, err);
            left *= right;
        }
        else if (**p == '/')
        {
            (*p)++;
            int right = eval_factor(p, err);
            if (right != 0)
            {
                left /= right;
            }
            else
            {
                *err = 2;
            }
        }
        else if (**p == '%')
        {
            (*p)++;
            int right = eval_factor(p, err);
            if (right != 0)
            {
                left %= right;
            }
            else
            {
                *err = 2;
            }
        }
        else
        {
            break;
        }
    }
    return left;
}

static int eval_expr(const char **p, int *err)
{
    int left = eval_term(p, err);
    while (1)
    {
        skip_spaces(p);
        if (**p == '+')
        {
            (*p)++;
            int right = eval_term(p, err);
            left += right;
        }
        else if (**p == '-')
        {
            (*p)++;
            int right = eval_term(p, err);
            left -= right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static int eval_condition(const char **p, int *err)
{
    int left = eval_expr(p, err);
    skip_spaces(p);
    char op[3] = {0, 0, 0};
    if ((**p == '=' && *(*p + 1) == '=') ||
        (**p == '!' && *(*p + 1) == '=') ||
        (**p == '<' && *(*p + 1) == '=') ||
        (**p == '>' && *(*p + 1) == '=') ||
        (**p == '<' && *(*p + 1) == '>'))
    {
        op[0] = **p;
        op[1] = *(*p + 1);
        *p += 2;
    }
    else if (**p == '=' || **p == '<' || **p == '>')
    {
        op[0] = **p;
        *p += 1;
    }
    else
    {
        *err = 3;
        return 0;
    }
    int right = eval_expr(p, err);
    if (op[1] == 0)
    {
        if (op[0] == '=')
            return left == right;
        if (op[0] == '<')
            return left < right;
        if (op[0] == '>')
            return left > right;
    }
    else
    {
        if (op[0] == '=' && op[1] == '=')
            return left == right;
        if (op[0] == '!' && op[1] == '=')
            return left != right;
        if (op[0] == '<' && op[1] == '>')
            return left != right;
        if (op[0] == '<' && op[1] == '=')
            return left <= right;
        if (op[0] == '>' && op[1] == '=')
            return left >= right;
    }
    return 0;
}

int main(void)
{
    int err = 0;
    vars[0] = 10;
    vars[1] = 2;

    const char *e1 = "A * 3 + B * 5";
    int r1 = eval_expr(&e1, &err);
    assert(err == 0 && r1 == 40);

    const char *e2 = "(A + 10) / (B + 2)";
    int r2 = eval_expr(&e2, &err);
    assert(err == 0 && r2 == 5);

    const char *c1 = "A == 10";
    assert(eval_condition(&c1, &err) == 1);
    const char *c2 = "A < B";
    assert(eval_condition(&c2, &err) == 0);
    const char *c3 = "A >= 10";
    assert(eval_condition(&c3, &err) == 1);
    const char *c4 = "B != 2";
    assert(eval_condition(&c4, &err) == 0);

    const char *k1 = "PRINT \"HELLO\"";
    assert(match_keyword(&k1, "PRINT") == 1);
    const char *k2 = "PRINTER";
    assert(match_keyword(&k2, "PRINT") == 0);

    printf("ALL BASIC TESTS PASSED\n");
    return 0;
}
