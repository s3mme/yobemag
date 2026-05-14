#include "cpu_randomized.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "cpu.h"
#include "mmu.h"

void cpu_randomized_run(const SM83TestCase *tc) {
    mmu_zero();
    cpu_init();

    for (int i = 0; i < tc->initial.ram_count; i++) {
        mmu_write_byte(tc->initial.ram[i].addr, tc->initial.ram[i].val);
    }

    CPU_REG_A = tc->initial.a;
    CPU_REG_B = tc->initial.b;
    CPU_REG_C = tc->initial.c;
    CPU_REG_D = tc->initial.d;
    CPU_REG_E = tc->initial.e;
    CPU_REG_F = tc->initial.f;
    CPU_REG_H = tc->initial.h;
    CPU_REG_L = tc->initial.l;
    cpu.SP    = tc->initial.sp;
    cpu.PC    = (uint16_t) (tc->initial.pc - 1u); // point to the opcode byte

    cpu_step();

    uint16_t want_pc = (uint16_t) (tc->final.pc - 1u);
    cr_expect(eq(u16, cpu.PC, want_pc), "[%s] PC: got 0x%04x, want 0x%04x", tc->name, cpu.PC, want_pc);
    cr_expect(eq(u8, CPU_REG_A, tc->final.a), "[%s] A", tc->name);
    cr_expect(eq(u8, CPU_REG_B, tc->final.b), "[%s] B", tc->name);
    cr_expect(eq(u8, CPU_REG_C, tc->final.c), "[%s] C", tc->name);
    cr_expect(eq(u8, CPU_REG_D, tc->final.d), "[%s] D", tc->name);
    cr_expect(eq(u8, CPU_REG_E, tc->final.e), "[%s] E", tc->name);
    cr_expect(eq(u8, CPU_REG_F, tc->final.f), "[%s] F", tc->name);
    cr_expect(eq(u8, CPU_REG_H, tc->final.h), "[%s] H", tc->name);
    cr_expect(eq(u8, CPU_REG_L, tc->final.l), "[%s] L", tc->name);
    cr_expect(eq(u16, cpu.SP, tc->final.sp), "[%s] SP", tc->name);

    for (int i = 0; i < tc->final.ram_count; i++) {
        uint16_t addr = tc->final.ram[i].addr;
        uint8_t want  = tc->final.ram[i].val;
        uint8_t got   = mmu_get_byte(addr);
        cr_expect(eq(u8, got, want), "[%s] mem[0x%04x]: got 0x%02x, want 0x%02x", tc->name, addr, got, want);
    }
}
