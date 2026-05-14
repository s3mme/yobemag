#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "../common/util.h"
#include "cpu.h"
#include "mmu.h"
#include "alu.h"

void emulate_instruction(const TestParams *const params) {
    // setup cpu
    uint16_t address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    cpu.PC           = address;
    CPU_REG_A        = params->lhs;

    // write rhs to desired register
    uint8_t *target_reg = get_cpu_reg(params->rhs_dreg_offset, params->rhs_reg_offset);
    *target_reg         = params->rhs;

    mmu_write_byte(address, params->opcode);

    // do the actual emulation
    cpu_step();

    // check flag register
    cr_expect(eq(u8, CPU_REG_F, params->F), "l: %d, r: %d, op: 0x%x", params->lhs, params->rhs, params->opcode);

    // check if value is correct
    uint8_t actual = CPU_REG_A;
    cr_expect(eq(u8, actual, params->expected), "l: %d, r: %d, op: 0x%x", params->lhs, params->rhs, params->opcode);

    // check if PC is updated correctly
    cr_expect(eq(u16, cpu.PC, (address + params->address_increment)));
}

void emulate_HL_d8_instruction(const SpecialTestParams *const params) {
    uint8_t opcode             = params->opcode;
    uint8_t A                  = params->lhs;
    uint8_t value              = params->rhs;
    uint16_t address           = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
    uint16_t address_increment = 1;

    // setup cpu
    cpu.PC    = address;
    CPU_REG_A = A;
    mmu_write_byte(address, opcode);
    if (params->is_HL) {
        uint16_t indirection_address = (random() % (MEM_SIZE - ROM_LIMIT)) + ROM_LIMIT;
        CPU_DREG_HL                  = indirection_address;
        mmu_write_byte(indirection_address, value);
    } else {
        mmu_write_byte(address + 1, value);
        ++address_increment;
    }

    // do the actual emulation
    cpu_step();

    // check flag register
    cr_expect(eq(u8, CPU_REG_F, params->F), "l: %d, r: %d, op: 0x%x, uses_borrow: %d", params->lhs, params->rhs,
              params->opcode, params->uses_borrow);

    // check if value is correct
    cr_expect(eq(u8, CPU_REG_A, params->expected), "l: %d, r: %d, op: 0x%x, uses_borrow: %d", params->lhs, params->rhs,
              params->opcode, params->uses_borrow);

    // check if PC is updated correctly
    cr_expect(eq(u16, cpu.PC, address + address_increment));
}
