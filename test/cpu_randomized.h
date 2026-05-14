#ifndef YOBEMAG_CPU_RANDOMIZED_H
#define YOBEMAG_CPU_RANDOMIZED_H

#include "json.h"

/* Load tc->initial into CPU + memory, call cpu_step(), and
 * cr_expect-assert every field against tc->final. */
void cpu_randomized_run(const SM83TestCase *tc);

#endif // YOBEMAG_CPU_RANDOMIZED_H
