#include "json.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *buf;
    size_t pos;
    size_t len;
} Parser;

static void skip_ws(Parser *p) {
    while (p->pos < p->len && isspace((unsigned char) p->buf[p->pos])) {
        p->pos++;
    }
}

static int expect_char(Parser *p, char c) {
    skip_ws(p);
    if (p->pos >= p->len || p->buf[p->pos] != c) {
        return -1;
    }
    p->pos++;
    return 0;
}

static int parse_uint(Parser *p, unsigned int *out) {
    skip_ws(p);
    if (p->pos >= p->len || !isdigit((unsigned char) p->buf[p->pos])) {
        return -1;
    }
    *out = 0;
    while (p->pos < p->len && isdigit((unsigned char) p->buf[p->pos])) {
        *out = *out * 10u + (unsigned int) (p->buf[p->pos++] - '0');
    }
    return 0;
}

// Advance past a quoted string; copies into out[0..max-1] when out != NULL
static int parse_string(Parser *p, char *out, size_t max) {
    skip_ws(p);
    if (p->pos >= p->len || p->buf[p->pos] != '"') {
        return -1;
    }
    p->pos++;
    size_t i = 0;
    while (p->pos < p->len && p->buf[p->pos] != '"') {
        if (out && i + 1 < max) {
            out[i++] = p->buf[p->pos];
        }
        p->pos++;
    }
    if (out && i < max) {
        out[i] = '\0';
    }
    if (p->pos >= p->len) {
        return -1;
    }
    p->pos++; /* closing " */
    return 0;
}

// Advance past "key":
static int parse_key(Parser *p, const char *expected) {
    char key[32];
    if (parse_string(p, key, sizeof(key)) != 0) {
        return -1;
    }
    if (strcmp(key, expected) != 0) {
        return -1;
    }
    return expect_char(p, ':');
}

// [[addr, val], ...]
static int parse_ram(Parser *p, SM83State *s) {
    if (expect_char(p, '[') != 0) {
        return -1;
    }
    s->ram_count = 0;
    skip_ws(p);
    if (p->pos < p->len && p->buf[p->pos] == ']') {
        p->pos++;
        return 0;
    }

    while (1) {
        if (s->ram_count >= SM83_MAX_RAM_ENTRIES) {
            return -1;
        }
        unsigned int addr, val;
        if (expect_char(p, '[') != 0) {
            return -1;
        }
        if (parse_uint(p, &addr) != 0) {
            return -1;
        }
        if (expect_char(p, ',') != 0) {
            return -1;
        }
        if (parse_uint(p, &val) != 0) {
            return -1;
        }
        if (expect_char(p, ']') != 0) {
            return -1;
        }
        s->ram[s->ram_count].addr = (uint16_t) addr;
        s->ram[s->ram_count].val  = (uint8_t) val;
        s->ram_count++;
        skip_ws(p);
        if (p->pos >= p->len) {
            return -1;
        }
        if (p->buf[p->pos] == ']') {
            p->pos++;
            break;
        }
        if (p->buf[p->pos] == ',') {
            p->pos++;
            continue;
        }
        return -1;
    }
    return 0;
}

// { "a": N, "b": N, ... "ram": [[...]] }
static int parse_state(Parser *p, SM83State *s) {
    unsigned int tmp;
    if (expect_char(p, '{') != 0) {
        return -1;
    }

    if (parse_key(p, "a") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->a = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "b") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->b = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "c") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->c = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "d") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->d = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "e") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->e = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "f") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->f = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "h") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->h = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "l") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->l = (uint8_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "pc") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->pc = (uint16_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "sp") != 0) {
        return -1;
    }
    if (parse_uint(p, &tmp) != 0) {
        return -1;
    }
    s->sp = (uint16_t) tmp;
    if (expect_char(p, ',') != 0) {
        return -1;
    }

    if (parse_key(p, "ram") != 0) {
        return -1;
    }
    if (parse_ram(p, s) != 0) {
        return -1;
    }

    return expect_char(p, '}');
}

// Skip the cycles array, mixed-type entries ([addr, val, "read"]) are not used
static int skip_cycles(Parser *p) {
    if (expect_char(p, '[') != 0) {
        return -1;
    }
    int depth = 1;
    while (p->pos < p->len && depth > 0) {
        char c = p->buf[p->pos++];
        if (c == '[') {
            depth++;
        } else if (c == ']') {
            depth--;
        } else if (c == '"') {
            while (p->pos < p->len && p->buf[p->pos] != '"') {
                p->pos++;
            }
            if (p->pos < p->len) {
                p->pos++;
            }
        }
    }
    return (depth == 0) ? 0 : -1;
}

static int parse_test_case(Parser *p, SM83TestCase *tc) {
    if (expect_char(p, '{') != 0) {
        return -1;
    }
    if (parse_key(p, "name") != 0) {
        return -1;
    }
    if (parse_string(p, tc->name, sizeof(tc->name)) != 0) {
        return -1;
    }
    if (expect_char(p, ',') != 0) {
        return -1;
    }
    if (parse_key(p, "initial") != 0) {
        return -1;
    }
    if (parse_state(p, &tc->initial) != 0) {
        return -1;
    }
    if (expect_char(p, ',') != 0) {
        return -1;
    }
    if (parse_key(p, "final") != 0) {
        return -1;
    }
    if (parse_state(p, &tc->final) != 0) {
        return -1;
    }
    if (expect_char(p, ',') != 0) {
        return -1;
    }
    if (parse_key(p, "cycles") != 0) {
        return -1;
    }
    if (skip_cycles(p) != 0) {
        return -1;
    }
    return expect_char(p, '}');
}

int json_load_cases(const char *path, SM83TestCase **out) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    if (fsize <= 0) {
        fclose(f);
        return -1;
    }

    char *buf = malloc((size_t) fsize + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }
    fread(buf, 1, (size_t) fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    Parser p            = {buf, 0, (size_t) fsize};
    size_t capacity     = 16;
    size_t count        = 0;
    SM83TestCase *cases = malloc(capacity * sizeof(SM83TestCase));
    if (!cases) {
        free(buf);
        return -1;
    }

    if (expect_char(&p, '[') != 0) {
        free(buf);
        free(cases);
        return -1;
    }

    while (1) {
        skip_ws(&p);
        if (p.pos >= p.len || p.buf[p.pos] == ']') {
            break;
        }

        if (count == capacity) {
            capacity *= 2;
            SM83TestCase *tmp = realloc(cases, capacity * sizeof(SM83TestCase));
            if (!tmp) {
                free(buf);
                free(cases);
                return -1;
            }
            cases = tmp;
        }

        if (parse_test_case(&p, &cases[count]) != 0) {
            free(buf);
            free(cases);
            return -1;
        }
        count++;

        skip_ws(&p);
        if (p.pos < p.len && p.buf[p.pos] == ',') {
            p.pos++;
        }
    }

    free(buf);
    *out = cases;
    return (int) count;
}
