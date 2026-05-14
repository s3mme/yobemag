#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/parameterized.h>

#include <stdbool.h>

#include "fixtures/cpu_mmu.h"
#include "common/util.h"

typedef struct JrNZTestParams {
    uint8_t opcode;
    int8_t immediate_value;
    Flag flag;
    uint8_t flag_value;
    bool branching_condition;
} JrNZTestParams;

ParameterizedTestParameters(jr_cc_n, jr_cc_n) {
    static JrNZTestParams params[] = {
  // take the branch - positive
        {0x20, 101, Z_FLAG, 0, true },
        {0x30, 101, C_FLAG, 0, true },
        {0x28, 101, Z_FLAG, 1, true },
        {0x38, 101, C_FLAG, 1, true },
 // take the branch - negative
        {0x20, -50, Z_FLAG, 0, true },
        {0x30, -50, C_FLAG, 0, true },
        {0x28, -50, Z_FLAG, 1, true },
        {0x38, -50, C_FLAG, 1, true },

 // don't take the branch - positive
        {0x20, 101, Z_FLAG, 1, false},
        {0x30, 101, C_FLAG, 1, false},
        {0x28, 101, Z_FLAG, 0, false},
        {0x38, 101, C_FLAG, 0, false},

 // don't take the branch - negative
        {0x20, -50, Z_FLAG, 1, false},
        {0x30, -50, C_FLAG, 1, false},
        {0x28, -50, Z_FLAG, 0, false},
        {0x38, -50, C_FLAG, 0, false},
    };

    // generate parameter set
    return cr_make_param_array(JrNZTestParams, params, sizeof(params) / sizeof(JrNZTestParams));
}

ParameterizedTest(JrNZTestParams *params, jr_cc_n, jr_cc_n, .init = cpu_mmu_setup, .fini = cpu_teardown) {
    // setup cpu
    uint16_t opcode_address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    cpu.PC                  = opcode_address;

    // set_flag(0,*) does nothing, but the flag is still cleared due to clear_flag_register()
    clear_flag_register();
    set_flag(params->flag_value, params->flag);

    mmu_write_byte(opcode_address, params->opcode);
    mmu_write_byte(opcode_address + 1, params->immediate_value);

    // do the actual emulation
    cpu_step();

    // check if PC is correct
    int8_t branching_addition;
    if (params->branching_condition) {
        branching_addition = params->immediate_value;
    } else {
        branching_addition = 0;
    }

    cr_expect(eq(u16, cpu.PC, opcode_address + 2 + branching_addition));
}
