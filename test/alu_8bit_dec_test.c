#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/parameterized.h>

#include "fixtures/cpu_mmu.h"
#include "common/util.h"
#include "common/alu.h"

static void emulate(const TestParams *const params) {
    // setup cpu
    uint16_t address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    cpu.PC           = address;

    // write rhs to desired register
    uint8_t *target_reg = get_cpu_reg(params->rhs_dreg_offset, params->rhs_reg_offset);
    *target_reg         = params->rhs;

    mmu_write_byte(address, params->opcode);

    // do the actual emulation
    cpu_step();

    // check flag register
    cr_expect(eq(u8, CPU_REG_F, params->F), "l: %d, r: %d, op: 0x%x", params->lhs, params->rhs, params->opcode);

    // check if value is correct
    uint8_t actual = *target_reg;
    cr_expect(eq(u8, actual, params->expected), "l: %d, r: %d, op: 0x%x", params->lhs, params->rhs, params->opcode);

    // check if PC is updated correctly
    cr_expect(eq(u16, cpu.PC, (address + params->address_increment)));
}

void emulate_HL(const SpecialTestParams *const params) {
    uint8_t opcode             = params->opcode;
    uint8_t value              = params->rhs;
    uint16_t address           = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    uint16_t address_increment = 1;

    // setup cpu
    cpu.PC = address;
    mmu_write_byte(address, opcode);
    uint16_t indirection_address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    CPU_DREG_HL                  = indirection_address;
    mmu_write_byte(indirection_address, value);

    // do the actual emulation
    cpu_step();

    // check flag register
    cr_expect(eq(u8, CPU_REG_F, params->F), "l: %d, r: %d, op: 0x%x, uses_borrow: %d", params->lhs, params->rhs,
              params->opcode, params->uses_borrow);

    // check if value is correct
    uint8_t actual = mmu_get_byte(indirection_address);
    cr_expect(eq(u8, actual, params->expected), "l: %d, r: %d, op: 0x%x, uses_borrow: %d", params->lhs, params->rhs,
              params->opcode, params->uses_borrow);

    // check if PC is updated correctly
    cr_expect(eq(u16, cpu.PC, address + address_increment));
}

ParameterizedTestParameters(DEC_n, DEC_n_half_carry) {
    static TestParams params[] = {
        {0x05, 0, 16, offsetof(CPU, BC), offsetof(DoubleWordReg, words.hi), 15, 1, 0b01100000},
        {0x0D, 0, 16, offsetof(CPU, BC), offsetof(DoubleWordReg, words.lo), 15, 1, 0b01100000},
        {0x15, 0, 16, offsetof(CPU, DE), offsetof(DoubleWordReg, words.hi), 15, 1, 0b01100000},
        {0x1D, 0, 16, offsetof(CPU, DE), offsetof(DoubleWordReg, words.lo), 15, 1, 0b01100000},
        {0x25, 0, 16, offsetof(CPU, HL), offsetof(DoubleWordReg, words.hi), 15, 1, 0b01100000},
        {0x2D, 0, 16, offsetof(CPU, HL), offsetof(DoubleWordReg, words.lo), 15, 1, 0b01100000},
        {0x3D, 0, 16, offsetof(CPU, AF), offsetof(DoubleWordReg, words.hi), 15, 1, 0b01100000},
    };

    return cr_make_param_array(TestParams, params, sizeof(params) / sizeof(TestParams));
}

ParameterizedTest(TestParams *params, DEC_n, DEC_n_half_carry, .init = cpu_mmu_setup, .fini = cpu_teardown) {
    clear_flag_register();
    emulate(params);
}

ParameterizedTestParameters(DEC_n, DEC_n_zero) {
    static TestParams params[] = {
        {0x05, 0, 1, offsetof(CPU, BC), offsetof(DoubleWordReg, words.hi), 0, 1, 0b11000000},
        {0x0D, 0, 1, offsetof(CPU, BC), offsetof(DoubleWordReg, words.lo), 0, 1, 0b11000000},
        {0x15, 0, 1, offsetof(CPU, DE), offsetof(DoubleWordReg, words.hi), 0, 1, 0b11000000},
        {0x1D, 0, 1, offsetof(CPU, DE), offsetof(DoubleWordReg, words.lo), 0, 1, 0b11000000},
        {0x25, 0, 1, offsetof(CPU, HL), offsetof(DoubleWordReg, words.hi), 0, 1, 0b11000000},
        {0x2D, 0, 1, offsetof(CPU, HL), offsetof(DoubleWordReg, words.lo), 0, 1, 0b11000000},
        {0x3D, 0, 1, offsetof(CPU, AF), offsetof(DoubleWordReg, words.hi), 0, 1, 0b11000000},
    };

    return cr_make_param_array(TestParams, params, sizeof(params) / sizeof(TestParams));
}

ParameterizedTest(TestParams *params, DEC_n, DEC_n_zero, .init = cpu_mmu_setup, .fini = cpu_teardown) {
    clear_flag_register();
    emulate(params);
}

ParameterizedTestParameters(DEC_n, DEC_n_HL) {
    static SpecialTestParams params[] = {
        {0x35, 0, 1,  0,  0b11000000, true, false},
        {0x35, 0, 16, 15, 0b01100000, true, false},
    };

    return cr_make_param_array(SpecialTestParams, params, sizeof(params) / sizeof(SpecialTestParams));
}

ParameterizedTest(SpecialTestParams *params, DEC_n, DEC_n_HL, .init = cpu_mmu_setup, .fini = cpu_teardown) {
    clear_flag_register();
    emulate_HL(params);
}
