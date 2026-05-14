#ifndef YOBEMAG_JSON_H
#define YOBEMAG_JSON_H

#include <stdint.h>

#define SM83_MAX_RAM_ENTRIES 16

typedef struct {
    uint16_t addr;
    uint8_t val;
} SM83RamEntry;

typedef struct {
    uint8_t a, b, c, d, e, f, h, l;
    uint16_t pc, sp;
    SM83RamEntry ram[SM83_MAX_RAM_ENTRIES];
    int ram_count;
} SM83State;

typedef struct {
    char name[64];
    SM83State initial;
    SM83State final;
} SM83TestCase;

/* Parse all test cases from one v2 .json file.
 * Returns the count and sets *out to a malloc'd array. Caller must free(*out).
 * Returns -1 on error. */
int json_load_cases(const char *path, SM83TestCase **out);

#endif // YOBEMAG_JSON_H
