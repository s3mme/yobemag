#include <criterion/criterion.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_randomized.h"
#include "json.h"

Test(cpu_randomized, all_v2) {
    DIR *dir = opendir(CPU_RANDOMIZED_TEST_DIR);
    cr_assert_not_null(dir, "Cannot open test/json dir: " CPU_RANDOMIZED_TEST_DIR);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len < 5 || strcmp(entry->d_name + len - 5, ".json") != 0) {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", CPU_RANDOMIZED_TEST_DIR, entry->d_name);

        SM83TestCase *cases = NULL;
        int count           = json_load_cases(path, &cases);
        cr_assert(count > 0, "Failed to parse %s", path);

        for (int i = 0; i < count; i++) {
            cpu_randomized_run(&cases[i]);
        }

        free(cases);
    }

    closedir(dir);
}
