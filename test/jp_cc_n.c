#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/parameterized.h>

#include <stdbool.h>

#include "fixtures/cpu_mmu.h"
#include "common/util.h"

typedef struct JrNZTestParams {
    uint8_t opcode;
    Flag flag;
    uint8_t flag_value;
    bool branching_condition;
} JrNZTestParams;

ParameterizedTestParameters(jp_cc_n, jp_cc_n) {
    static JrNZTestParams params[] = {
  // take the branch - positive
        {0xC2, Z_FLAG, 0, true },
        {0xD2, C_FLAG, 0, true },
        {0xCA, Z_FLAG, 1, true },
        {0xDA, C_FLAG, 1, true },

 // don't take the branch
        {0xC2, Z_FLAG, 1, false},
        {0xD2, C_FLAG, 1, false},
        {0xCA, Z_FLAG, 0, false},
        {0xDA, C_FLAG, 0, false},
    };

    // generate parameter set
    return cr_make_param_array(JrNZTestParams, params, sizeof(params) / sizeof(JrNZTestParams));
}

ParameterizedTest(JrNZTestParams *params, jp_cc_n, jp_cc_n, .init = cpu_mmu_setup, .fini = cpu_teardown) {
    // setup cpu
    uint16_t opcode_address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    uint16_t expected       = 0x1337;
    cpu.PC                  = opcode_address;

    // set_flag(0,*) does nothing, but the flag is still cleared due to clear_flag_register()
    clear_flag_register();
    set_flag(params->flag_value, params->flag);

    mmu_write_byte(opcode_address, params->opcode);
    mmu_write_two_bytes(opcode_address + 1, expected);

    // do the actual emulation
    cpu_step();

    // check if PC is correct
    int8_t branching_addition;
    if (params->branching_condition) {
        cr_expect(eq(u16, cpu.PC, expected));
    } else {
        cr_expect(eq(u16, cpu.PC, opcode_address + 3));
    }
}
