#include <stdint.h>

#include "mmu.h"
#include "cpu.h"
#include "log.h"

#define LO_NIBBLE_MASK (0x0F)
#define HI_NIBBLE_MASK (0xF0)
#define BYTE_MASK      (0xFF)

typedef void (*op_function)(void);

// Function is used for instruction array initialization, not recognized by compiler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#if defined(YOBEMAG_DEBUG)

static void UNKNOWN_OPCODE(void) {
    static unsigned err_count = 0;
    LOG_ERROR("Unallowed/Unimplemented OP Code: 0x%x", cpu.opcode);
    if (++err_count > 100) {
        YOBEMAG_EXIT("Exceeded number of unallowed OP codes!");
    }
}

#else

_Noreturn static void UNKNOWN_OPCODE(void) {
    YOBEMAG_EXIT("An unsupported instruction was encountered: 0x%x", cpu.opcode);
}

#endif // defined(YOBEMAG_EXIT)

#pragma GCC diagnostic pop

// internal prototypes
void OPC_RST_x(uint16_t address);
void OPC_LD_xx_u16(uint16_t load_addr);
static void cb_optable_init(void);

static op_function instr_lookup[0xFF + 1]       = {[0 ... 0xFF] = &UNKNOWN_OPCODE};
static op_function cb_prefixed_lookup[0xFF + 1] = {[0 ... 0xFF] = &UNKNOWN_OPCODE};

CPU cpu = {
    .HL.dword = 0,
    .DE.dword = 0,
    .BC.dword = 0,
    .AF.dword = 0,
    0,
    0,
    0,
    0,
};

__attribute__((always_inline)) inline static void LD_REG_REG(uint8_t *register_one, uint8_t register_two) {
    *register_one = register_two;
}

static void LD_REG_d8(uint8_t *reg_addr) {
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    LD_REG_REG(reg_addr, immediate);
}

static void optable_init(void) {
    // Set up lookup table
    instr_lookup[0x00] = OPC_NOP;
    instr_lookup[0xF3] = OPC_DI;
    instr_lookup[0x03] = OPC_INC_BC;

    instr_lookup[0x31] = OPC_LD_SP;

    // misc
    instr_lookup[0x20] = OPC_JR_NZ;
    instr_lookup[0x30] = OPC_JR_NC;
    instr_lookup[0x28] = OPC_JR_Z;
    instr_lookup[0x38] = OPC_JR_C;

    instr_lookup[0xC2] = OPC_JP_NZ;
    instr_lookup[0xD2] = OPC_JP_NC;
    instr_lookup[0xCA] = OPC_JP_Z;
    instr_lookup[0xDA] = OPC_JP_C;

    // 8-bit loads
    instr_lookup[0x02] = OPC_LD_BC_A;
    instr_lookup[0x12] = OPC_LD_DE_A;
    instr_lookup[0x22] = OPC_LD_HL_PLUS_A;
    instr_lookup[0x32] = OPC_LD_HL_MINUS_A;

    instr_lookup[0x0A] = OPC_LD_A_BC;
    instr_lookup[0x1A] = OPC_LD_A_DE;
    instr_lookup[0x2A] = OPC_LD_A_HL_PLUS;
    instr_lookup[0x3A] = OPC_LD_A_HL_MINUS;

    instr_lookup[0xE0] = OPC_LD_FF00a8_A;
    instr_lookup[0xE2] = OPC_LD_FF00C_A;
    instr_lookup[0xF0] = OPC_LD_A_FF00a8;
    instr_lookup[0xF2] = OPC_LD_A_FF00C;

    instr_lookup[0xEA] = OPC_LD_a16_A;
    instr_lookup[0xFA] = OPC_LD_A_a16;

    instr_lookup[0x40] = OPC_LD_B_B;
    instr_lookup[0x41] = OPC_LD_B_C;
    instr_lookup[0x42] = OPC_LD_B_D;
    instr_lookup[0x43] = OPC_LD_B_E;
    instr_lookup[0x44] = OPC_LD_B_H;
    instr_lookup[0x45] = OPC_LD_B_L;
    instr_lookup[0x46] = OPC_LD_B_HL;
    instr_lookup[0x47] = OPC_LD_B_A;
    instr_lookup[0x06] = OPC_LD_B_d8;

    instr_lookup[0x48] = OPC_LD_C_B;
    instr_lookup[0x49] = OPC_LD_C_C;
    instr_lookup[0x4A] = OPC_LD_C_D;
    instr_lookup[0x4B] = OPC_LD_C_E;
    instr_lookup[0x4C] = OPC_LD_C_H;
    instr_lookup[0x4D] = OPC_LD_C_L;
    instr_lookup[0x4E] = OPC_LD_C_HL;
    instr_lookup[0x4F] = OPC_LD_C_A;
    instr_lookup[0x0E] = OPC_LD_C_d8;

    instr_lookup[0x50] = OPC_LD_D_B;
    instr_lookup[0x51] = OPC_LD_D_C;
    instr_lookup[0x52] = OPC_LD_D_D;
    instr_lookup[0x53] = OPC_LD_D_E;
    instr_lookup[0x54] = OPC_LD_D_H;
    instr_lookup[0x55] = OPC_LD_D_L;
    instr_lookup[0x56] = OPC_LD_D_HL;
    instr_lookup[0x57] = OPC_LD_D_A;
    instr_lookup[0x16] = OPC_LD_D_d8;

    instr_lookup[0x58] = OPC_LD_E_B;
    instr_lookup[0x59] = OPC_LD_E_C;
    instr_lookup[0x5A] = OPC_LD_E_D;
    instr_lookup[0x5B] = OPC_LD_E_E;
    instr_lookup[0x5C] = OPC_LD_E_H;
    instr_lookup[0x5D] = OPC_LD_E_L;
    instr_lookup[0x5E] = OPC_LD_E_HL;
    instr_lookup[0x5F] = OPC_LD_E_A;
    instr_lookup[0x1E] = OPC_LD_E_d8;

    instr_lookup[0x60] = OPC_LD_H_B;
    instr_lookup[0x61] = OPC_LD_H_C;
    instr_lookup[0x62] = OPC_LD_H_D;
    instr_lookup[0x63] = OPC_LD_H_E;
    instr_lookup[0x64] = OPC_LD_H_H;
    instr_lookup[0x65] = OPC_LD_H_L;
    instr_lookup[0x66] = OPC_LD_H_HL;
    instr_lookup[0x67] = OPC_LD_H_A;
    instr_lookup[0x26] = OPC_LD_H_d8;

    instr_lookup[0x68] = OPC_LD_L_B;
    instr_lookup[0x69] = OPC_LD_L_C;
    instr_lookup[0x6A] = OPC_LD_L_D;
    instr_lookup[0x6B] = OPC_LD_L_E;
    instr_lookup[0x6C] = OPC_LD_L_H;
    instr_lookup[0x6D] = OPC_LD_L_L;
    instr_lookup[0x6E] = OPC_LD_L_HL;
    instr_lookup[0x6F] = OPC_LD_L_A;
    instr_lookup[0x2E] = OPC_LD_L_d8;

    instr_lookup[0x70] = OPC_LD_HL_B;
    instr_lookup[0x71] = OPC_LD_HL_C;
    instr_lookup[0x72] = OPC_LD_HL_D;
    instr_lookup[0x73] = OPC_LD_HL_E;
    instr_lookup[0x74] = OPC_LD_HL_H;
    instr_lookup[0x75] = OPC_LD_HL_L;
    instr_lookup[0x77] = OPC_LD_HL_A;
    instr_lookup[0x36] = OPC_LD_HL_d8;

    instr_lookup[0x78] = OPC_LD_A_B;
    instr_lookup[0x79] = OPC_LD_A_C;
    instr_lookup[0x7A] = OPC_LD_A_D;
    instr_lookup[0x7B] = OPC_LD_A_E;
    instr_lookup[0x7C] = OPC_LD_A_H;
    instr_lookup[0x7D] = OPC_LD_A_L;
    instr_lookup[0x7E] = OPC_LD_A_HL;
    instr_lookup[0x7F] = OPC_LD_A_A;
    instr_lookup[0x3E] = OPC_LD_A_d8;

    // 8-bit ALU: ADD A,n
    instr_lookup[0x80] = OPC_ADD_A_B;
    instr_lookup[0x81] = OPC_ADD_A_C;
    instr_lookup[0x82] = OPC_ADD_A_D;
    instr_lookup[0x83] = OPC_ADD_A_E;
    instr_lookup[0x84] = OPC_ADD_A_H;
    instr_lookup[0x85] = OPC_ADD_A_L;
    instr_lookup[0x86] = OPC_ADD_A_HL;
    instr_lookup[0x87] = OPC_ADD_A_A;
    instr_lookup[0xC6] = OPC_ADD_A_d8;

    // 8-bit ALU: ADC A,n
    instr_lookup[0x88] = OPC_ADC_A_B;
    instr_lookup[0x89] = OPC_ADC_A_C;
    instr_lookup[0x8A] = OPC_ADC_A_D;
    instr_lookup[0x8B] = OPC_ADC_A_E;
    instr_lookup[0x8C] = OPC_ADC_A_H;
    instr_lookup[0x8D] = OPC_ADC_A_L;
    instr_lookup[0x8E] = OPC_ADC_A_HL;
    instr_lookup[0x8F] = OPC_ADC_A_A;
    instr_lookup[0xCE] = OPC_ADC_A_d8;

    // 8-bit ALU: SUB A,n
    instr_lookup[0x90] = OPC_SUB_A_B;
    instr_lookup[0x91] = OPC_SUB_A_C;
    instr_lookup[0x92] = OPC_SUB_A_D;
    instr_lookup[0x93] = OPC_SUB_A_E;
    instr_lookup[0x94] = OPC_SUB_A_H;
    instr_lookup[0x95] = OPC_SUB_A_L;
    instr_lookup[0x96] = OPC_SUB_A_HL;
    instr_lookup[0x97] = OPC_SUB_A_A;
    instr_lookup[0xD6] = OPC_SUB_A_d8;

    // 8-bit ALU: SUB A,n
    instr_lookup[0x98] = OPC_SBC_A_B;
    instr_lookup[0x99] = OPC_SBC_A_C;
    instr_lookup[0x9A] = OPC_SBC_A_D;
    instr_lookup[0x9B] = OPC_SBC_A_E;
    instr_lookup[0x9C] = OPC_SBC_A_H;
    instr_lookup[0x9D] = OPC_SBC_A_L;
    instr_lookup[0x9E] = OPC_SBC_A_HL;
    instr_lookup[0x9F] = OPC_SBC_A_A;
    instr_lookup[0xDE] = OPC_SBC_A_d8;

    // 8-bit ALU: AND A,n
    instr_lookup[0xA0] = OPC_AND_A_B;
    instr_lookup[0xA1] = OPC_AND_A_C;
    instr_lookup[0xA2] = OPC_AND_A_D;
    instr_lookup[0xA3] = OPC_AND_A_E;
    instr_lookup[0xA4] = OPC_AND_A_H;
    instr_lookup[0xA5] = OPC_AND_A_L;
    instr_lookup[0xA6] = OPC_AND_A_HL;
    instr_lookup[0xA7] = OPC_AND_A_A;
    instr_lookup[0xE6] = OPC_AND_A_d8;

    // 8-bit ALU: OR A,n
    instr_lookup[0xB0] = OPC_OR_A_B;
    instr_lookup[0xB1] = OPC_OR_A_C;
    instr_lookup[0xB2] = OPC_OR_A_D;
    instr_lookup[0xB3] = OPC_OR_A_E;
    instr_lookup[0xB4] = OPC_OR_A_H;
    instr_lookup[0xB5] = OPC_OR_A_L;
    instr_lookup[0xB6] = OPC_OR_A_HL;
    instr_lookup[0xB7] = OPC_OR_A_A;
    instr_lookup[0xF6] = OPC_OR_A_d8;

    // 8-bit ALU: XOR A,n
    instr_lookup[0xA8] = OPC_XOR_A_B;
    instr_lookup[0xA9] = OPC_XOR_A_C;
    instr_lookup[0xAA] = OPC_XOR_A_D;
    instr_lookup[0xAB] = OPC_XOR_A_E;
    instr_lookup[0xAC] = OPC_XOR_A_H;
    instr_lookup[0xAD] = OPC_XOR_A_L;
    instr_lookup[0xAE] = OPC_XOR_A_HL;
    instr_lookup[0xAF] = OPC_XOR_A_A;
    instr_lookup[0xEE] = OPC_XOR_A_d8;

    // 8-bit ALU: CP A,n
    instr_lookup[0xB8] = OPC_CP_A_B;
    instr_lookup[0xB9] = OPC_CP_A_C;
    instr_lookup[0xBA] = OPC_CP_A_D;
    instr_lookup[0xBB] = OPC_CP_A_E;
    instr_lookup[0xBC] = OPC_CP_A_H;
    instr_lookup[0xBD] = OPC_CP_A_L;
    instr_lookup[0xBE] = OPC_CP_A_HL;
    instr_lookup[0xBF] = OPC_CP_A_A;
    instr_lookup[0xFE] = OPC_CP_A_d8;

    // 8-bit: ALU: INC n
    instr_lookup[0x04] = OPC_INC_B;
    instr_lookup[0x0C] = OPC_INC_C;
    instr_lookup[0x14] = OPC_INC_D;
    instr_lookup[0x1C] = OPC_INC_E;
    instr_lookup[0x24] = OPC_INC_H;
    instr_lookup[0x2C] = OPC_INC_L;
    instr_lookup[0x34] = OPC_INC_HL;
    instr_lookup[0x3C] = OPC_INC_A;

    // 8-bit ALU: DEC n
    instr_lookup[0x05] = OPC_DEC_B;
    instr_lookup[0x0D] = OPC_DEC_C;
    instr_lookup[0x15] = OPC_DEC_D;
    instr_lookup[0x1D] = OPC_DEC_E;
    instr_lookup[0x25] = OPC_DEC_H;
    instr_lookup[0x2D] = OPC_DEC_L;
    instr_lookup[0x35] = OPC_DEC_HL;
    instr_lookup[0x3D] = OPC_DEC_A;

    instr_lookup[0xCF] = OPC_RST_08;
    instr_lookup[0xDF] = OPC_RST_18;
    instr_lookup[0xEF] = OPC_RST_28;
    instr_lookup[0xFF] = OPC_RST_38;

    // LD, XX u16
    instr_lookup[0x01] = OPC_LD_BC_u16;
    instr_lookup[0x11] = OPC_LD_DE_u16;
    instr_lookup[0x21] = OPC_LD_HL_u16;
    instr_lookup[0x31] = OPC_LD_SP_u16;

    // JR i8
    instr_lookup[0x18] = OPC_JR_i8;

    // CALL u16
    instr_lookup[0xcd] = OPC_CALL_u16;

    // CALL cc, u16
    instr_lookup[0xc4] = OPC_CALL_NZ_u16;
    instr_lookup[0xcc] = OPC_CALL_Z_u16;
    instr_lookup[0xd4] = OPC_CALL_NC_u16;
    instr_lookup[0xdc] = OPC_CALL_C_u16;

    // Misc / control
    instr_lookup[0x10] = OPC_STOP;
    instr_lookup[0x76] = OPC_HALT;
    instr_lookup[0xFB] = OPC_EI;
    instr_lookup[0x27] = OPC_DAA;
    instr_lookup[0x2F] = OPC_CPL;
    instr_lookup[0x37] = OPC_SCF;
    instr_lookup[0x3F] = OPC_CCF;

    // Rotates on A
    instr_lookup[0x07] = OPC_RLCA;
    instr_lookup[0x0F] = OPC_RRCA;
    instr_lookup[0x17] = OPC_RLA;
    instr_lookup[0x1F] = OPC_RRA;

    // 16-bit loads
    instr_lookup[0x08] = OPC_LD_a16_SP;
    instr_lookup[0xF8] = OPC_LD_HL_SP_i8;
    instr_lookup[0xF9] = OPC_LD_SP_HL;

    // 16-bit ALU: INC rr
    instr_lookup[0x13] = OPC_INC_DE;
    instr_lookup[0x23] = OPC_INC_HL16;
    instr_lookup[0x33] = OPC_INC_SP;

    // 16-bit ALU: DEC rr
    instr_lookup[0x0B] = OPC_DEC_BC;
    instr_lookup[0x1B] = OPC_DEC_DE;
    instr_lookup[0x2B] = OPC_DEC_HL16;
    instr_lookup[0x3B] = OPC_DEC_SP;

    // 16-bit ALU: ADD HL, rr
    instr_lookup[0x09] = OPC_ADD_HL_BC;
    instr_lookup[0x19] = OPC_ADD_HL_DE;
    instr_lookup[0x29] = OPC_ADD_HL_HL;
    instr_lookup[0x39] = OPC_ADD_HL_SP;

    // 16-bit ALU: ADD SP, i8
    instr_lookup[0xE8] = OPC_ADD_SP_i8;

    // Jumps
    instr_lookup[0xC3] = OPC_JP_a16;
    instr_lookup[0xE9] = OPC_JP_HL;

    // Stack: PUSH rr
    instr_lookup[0xC5] = OPC_PUSH_BC;
    instr_lookup[0xD5] = OPC_PUSH_DE;
    instr_lookup[0xE5] = OPC_PUSH_HL;
    instr_lookup[0xF5] = OPC_PUSH_AF;

    // Stack: POP rr
    instr_lookup[0xC1] = OPC_POP_BC;
    instr_lookup[0xD1] = OPC_POP_DE;
    instr_lookup[0xE1] = OPC_POP_HL;
    instr_lookup[0xF1] = OPC_POP_AF;

    // Returns
    instr_lookup[0xC9] = OPC_RET;
    instr_lookup[0xD9] = OPC_RETI;
    instr_lookup[0xC0] = OPC_RET_NZ;
    instr_lookup[0xC8] = OPC_RET_Z;
    instr_lookup[0xD0] = OPC_RET_NC;
    instr_lookup[0xD8] = OPC_RET_C;

    // Restarts
    instr_lookup[0xC7] = OPC_RST_00;
    instr_lookup[0xD7] = OPC_RST_10;
    instr_lookup[0xE7] = OPC_RST_20;
    instr_lookup[0xF7] = OPC_RST_30;

    cb_optable_init();
}

// ------------------ CPU Funcs
void cpu_init(void) {
    optable_init();

    CPU_DREG_AF     = 0x01B0;
    CPU_DREG_BC     = 0x0013;
    CPU_DREG_DE     = 0x00D8;
    CPU_DREG_HL     = 0x014D;
    cpu.SP          = 0xFFFE;
    cpu.PC          = 0x0100;
    cpu.cycle_count = 0;
}

void cpu_print_registers(void) {
    LOG_INFO("PC: %04X AF: %02X%02X, BC: %02X%02X, DE: %02X%02X, HL: %02X%02X, SP: %04X, cycles: %d", cpu.PC, CPU_REG_A,
             CPU_REG_F, CPU_REG_B, CPU_REG_C, CPU_REG_D, CPU_REG_E, CPU_REG_H, CPU_REG_L, cpu.SP, cpu.cycle_count);
}

void cpu_step(void) {
    LOG_DEBUG("PC 0x%04X", cpu.PC);
    LOG_DEBUG("MMU[PC]: 0x%04X", mmu_get_byte(cpu.PC));
    LOG_DEBUG("MMU[PC+1]: 0x%04X", mmu_get_byte(cpu.PC + 1));
    LOG_DEBUG("MMU[PC+2]: 0x%04X", mmu_get_byte(cpu.PC + 2));

    cpu.opcode = mmu_get_byte(cpu.PC++);

    // CB prefix: get second byte and run
    if (cpu.opcode == 0xCB) {
        cpu.opcode = mmu_get_byte(cpu.PC++);
        (*(cb_prefixed_lookup[cpu.opcode]))();
    } else {
        (*(instr_lookup[cpu.opcode]))();
    }
    // We cannot know (here) the exact number of increments that the PC and cycle count need,
    // hence the instructions themselves do it

    LOG_DEBUG("-----------------");
}

// OP-Codes
void OPC_NOP(void) {
    LOG_DEBUG("OPC_NOP(void)");
    ++cpu.cycle_count;
}

void OPC_INC_BC(void) {
    LOG_DEBUG("OPC_INC_BC(void)");
    ++CPU_DREG_BC;
    cpu.cycle_count += 2;
}

void OPC_LD_SP(void) {
    LOG_DEBUG("OPC_LD_SP(void)");
    cpu.SP = mmu_get_byte(cpu.PC);
    cpu.PC += 2;
    cpu.cycle_count += 3;
}

/******************************************************
 *** Misc                                           ***
 ******************************************************/
static void OPC_JR_cc_n(uint8_t bit, uint8_t branching_condition) {
    LOG_DEBUG("OPC_JR_cc_n(uint8_t bit, uint8_t branching_condition)");
    int8_t n = (int8_t) mmu_get_byte(cpu.PC + 1);

    // doing the PC increment before the opcode's override because we are emulating
    // two CPU cycles beforehand (fetching opcode in cpu_step, and fetching n above)
    cpu.PC += 2;

    if (bit == branching_condition) {
        cpu.PC = (uint16_t) (cpu.PC + n);
        cpu.cycle_count += 12;
    } else {
        cpu.cycle_count += 8;
    }
}

void OPC_JR_NZ(void) {
    LOG_DEBUG("OPC_JR_NZ(void)");
    OPC_JR_cc_n(get_flag(Z_FLAG), 0);
}

void OPC_JR_NC(void) {
    LOG_DEBUG("OPC_JR_NC(void)");
    OPC_JR_cc_n(get_flag(C_FLAG), 0);
}

void OPC_JR_Z(void) {
    LOG_DEBUG("OPC_JR_Z(void)");
    OPC_JR_cc_n(get_flag(Z_FLAG), 1);
}

void OPC_JR_C(void) {
    LOG_DEBUG("OPC_JR_C(void)");
    OPC_JR_cc_n(get_flag(C_FLAG), 1);
}

static void OPC_JP_cc_a16(uint8_t bit, uint8_t branching_condition) {
    LOG_DEBUG("OPC_JP_cc_a16(uint8_t bit, uint8_t branching_condition)");
    uint16_t n = mmu_get_two_bytes(cpu.PC + 1);

    if (bit == branching_condition) {
        cpu.PC = n;
        cpu.cycle_count += 16;
    } else {
        cpu.PC += 3;
        cpu.cycle_count += 12;
    }
}

void OPC_JP_NZ(void) {
    LOG_DEBUG("OPC_JP_NZ(void)");
    OPC_JP_cc_a16(get_flag(Z_FLAG), 0);
}

void OPC_JP_NC(void) {
    LOG_DEBUG("OPC_JP_NC(void)");
    OPC_JP_cc_a16(get_flag(C_FLAG), 0);
}

void OPC_JP_Z(void) {
    LOG_DEBUG("OPC_JP_Z(void)");
    OPC_JP_cc_a16(get_flag(Z_FLAG), 1);
}

void OPC_JP_C(void) {
    LOG_DEBUG("OPC_JP_C(void)");
    OPC_JP_cc_a16(get_flag(C_FLAG), 1);
}

/******************************************************
 *** 8-BIT Loads                                    ***
 ******************************************************/
void OPC_LD_BC_A(void) {
    LOG_DEBUG("OPC_LD_BC_A(void)");
    mmu_write_byte(CPU_DREG_BC, CPU_REG_A);

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_DE_A(void) {
    LOG_DEBUG("OPC_LD_DE_A(void)");
    mmu_write_byte(CPU_DREG_DE, CPU_REG_A);

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_HL_PLUS_A(void) {
    LOG_DEBUG("OPC_LD_HL_PLUS_A(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_A);
    ++CPU_DREG_HL;

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_HL_MINUS_A(void) {
    LOG_DEBUG("OPC_LD_HL_MINUS_A(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_A);
    --CPU_DREG_HL;

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_A_BC(void) {
    LOG_DEBUG("OPC_LD_A_BC(void)");
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(CPU_DREG_BC));

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_A_DE(void) {
    LOG_DEBUG("OPC_LD_A_DE(void)");
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(CPU_DREG_DE));

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_A_HL_PLUS(void) {
    LOG_DEBUG("OPC_LD_A_HL_PLUS(void)");
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(CPU_DREG_HL));
    ++CPU_DREG_HL;

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_A_HL_MINUS(void) {
    LOG_DEBUG("OPC_LD_A_HL_MINUS(void)");
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(CPU_DREG_HL));
    --CPU_DREG_HL;

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_FF00a8_A(void) {
    LOG_DEBUG("OPC_LD_FF00a8_A(void)");
    uint16_t intermediate = 0xFF00 + mmu_get_byte(cpu.PC + 1);
    mmu_write_byte(intermediate, CPU_REG_A);

    cpu.cycle_count += 12;
    cpu.PC += 2;
}

void OPC_LD_A_FF00a8(void) {
    LOG_DEBUG("OPC_LD_A_FF00a8(void)");
    uint16_t intermediate = 0xFF00 + mmu_get_byte(cpu.PC + 1);
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(intermediate));

    cpu.cycle_count += 12;
    cpu.PC += 2;
}

void OPC_LD_FF00C_A(void) {
    LOG_DEBUG("OPC_LD_FF00C_A(void)");
    uint16_t intermediate = 0xFF00 + CPU_REG_C;
    mmu_write_byte(intermediate, CPU_REG_A);

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_A_FF00C(void) {
    LOG_DEBUG("OPC_LD_A_FF00C(void)");
    uint16_t intermediate = 0xFF00 + CPU_REG_C;
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(intermediate));

    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_LD_a16_A(void) {
    LOG_DEBUG("OPC_LD_a16_A(void)");
    uint16_t intermediate = mmu_get_two_bytes(cpu.PC + 1);

    mmu_write_byte(intermediate, CPU_REG_A);

    cpu.cycle_count += 16;
    cpu.PC += 3;
}

void OPC_LD_A_a16(void) {
    LOG_DEBUG("OPC_LD_A_a16(void)");
    uint16_t intermediate = mmu_get_two_bytes(cpu.PC + 1);

    LD_REG_REG(&CPU_REG_A, mmu_get_byte(intermediate));

    cpu.cycle_count += 16;
    cpu.PC += 3;
}

// LD B, n
void OPC_LD_B_B(void) {
    LOG_DEBUG("OPC_LD_B_B(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_B_C(void) {
    LOG_DEBUG("OPC_LD_B_C(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_B_D(void) {
    LOG_DEBUG("OPC_LD_B_D(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_B_E(void) {
    LOG_DEBUG("OPC_LD_B_E(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_B_H(void) {
    LOG_DEBUG("OPC_LD_B_H(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_B_L(void) {
    LOG_DEBUG("OPC_LD_B_L(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_B_HL(void) {
    LOG_DEBUG("OPC_LD_B_HL(void)");
    LD_REG_REG(&CPU_REG_B, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_B_A(void) {
    LOG_DEBUG("OPC_LD_B_A(void)");
    LD_REG_REG(&CPU_REG_B, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_B_d8(void) {
    LOG_DEBUG("OPC_LD_B_d8(void)");
    LD_REG_d8(&CPU_REG_B);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD C, n
void OPC_LD_C_B(void) {
    LOG_DEBUG("OPC_LD_C_B(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_C_C(void) {
    LOG_DEBUG("OPC_LD_C_C(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_C_D(void) {
    LOG_DEBUG("OPC_LD_C_D(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_C_E(void) {
    LOG_DEBUG("OPC_LD_C_E(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_C_H(void) {
    LOG_DEBUG("OPC_LD_C_H(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_C_L(void) {
    LOG_DEBUG("OPC_LD_C_L(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_C_HL(void) {
    LOG_DEBUG("OPC_LD_C_HL(void)");
    LD_REG_REG(&CPU_REG_C, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_C_A(void) {
    LOG_DEBUG("OPC_LD_C_A(void)");
    LD_REG_REG(&CPU_REG_C, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_C_d8(void) {
    LOG_DEBUG("OPC_LD_C_d8(void)");
    LD_REG_d8(&CPU_REG_C);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD D, n
void OPC_LD_D_B(void) {
    LOG_DEBUG("OPC_LD_D_B(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_D_C(void) {
    LOG_DEBUG("OPC_LD_D_C(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_D_D(void) {
    LOG_DEBUG("OPC_LD_D_D(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_D_E(void) {
    LOG_DEBUG("OPC_LD_D_E(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_D_H(void) {
    LOG_DEBUG("OPC_LD_D_H(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_D_L(void) {
    LOG_DEBUG("OPC_LD_D_L(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_D_HL(void) {
    LOG_DEBUG("OPC_LD_D_HL(void)");
    LD_REG_REG(&CPU_REG_D, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_D_A(void) {
    LOG_DEBUG("OPC_LD_D_A(void)");
    LD_REG_REG(&CPU_REG_D, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_D_d8(void) {
    LOG_DEBUG("OPC_LD_D_d8(void)");
    LD_REG_d8(&CPU_REG_D);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD E, n
void OPC_LD_E_B(void) {
    LOG_DEBUG("OPC_LD_E_B(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_E_C(void) {
    LOG_DEBUG("OPC_LD_E_C(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_E_D(void) {
    LOG_DEBUG("OPC_LD_E_D(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_E_E(void) {
    LOG_DEBUG("OPC_LD_E_E(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_E_H(void) {
    LOG_DEBUG("OPC_LD_E_H(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_E_L(void) {
    LOG_DEBUG("OPC_LD_E_L(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_E_HL(void) {
    LOG_DEBUG("OPC_LD_E_HL(void)");
    LD_REG_REG(&CPU_REG_E, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_E_A(void) {
    LOG_DEBUG("OPC_LD_E_A(void)");
    LD_REG_REG(&CPU_REG_E, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_E_d8(void) {
    LOG_DEBUG("OPC_LD_E_d8(void)");
    LD_REG_d8(&CPU_REG_E);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD H, n
void OPC_LD_H_B(void) {
    LOG_DEBUG("OPC_LD_H_B(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_H_C(void) {
    LOG_DEBUG("OPC_LD_H_C(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_H_D(void) {
    LOG_DEBUG("OPC_LD_H_D(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_H_E(void) {
    LOG_DEBUG("OPC_LD_H_E(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_H_H(void) {
    LOG_DEBUG("OPC_LD_H_H(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_H_L(void) {
    LOG_DEBUG("OPC_LD_H_L(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_H_HL(void) {
    LOG_DEBUG("OPC_LD_H_HL(void)");
    LD_REG_REG(&CPU_REG_H, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_H_A(void) {
    LOG_DEBUG("OPC_LD_H_A(void)");
    LD_REG_REG(&CPU_REG_H, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_H_d8(void) {
    LOG_DEBUG("OPC_LD_H_d8(void)");
    LD_REG_d8(&CPU_REG_H);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD L, n
void OPC_LD_L_B(void) {
    LOG_DEBUG("OPC_LD_L_B(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_L_C(void) {
    LOG_DEBUG("OPC_LD_L_C(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_L_D(void) {
    LOG_DEBUG("OPC_LD_L_D(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_L_E(void) {
    LOG_DEBUG("OPC_LD_L_E(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_L_H(void) {
    LOG_DEBUG("OPC_LD_L_H(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_L_L(void) {
    LOG_DEBUG("OPC_LD_L_L(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_L_HL(void) {
    LOG_DEBUG("OPC_LD_L_HL(void)");
    LD_REG_REG(&CPU_REG_L, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_L_A(void) {
    LOG_DEBUG("OPC_LD_L_A(void)");
    LD_REG_REG(&CPU_REG_L, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_L_d8(void) {
    LOG_DEBUG("OPC_LD_L_d8(void)");
    LD_REG_d8(&CPU_REG_L);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

// LD HL, n
void OPC_LD_HL_B(void) {
    LOG_DEBUG("OPC_LD_HL_B(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_B);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_C(void) {
    LOG_DEBUG("OPC_LD_HL_C(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_C);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_D(void) {
    LOG_DEBUG("OPC_LD_HL_D(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_D);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_E(void) {
    LOG_DEBUG("OPC_LD_HL_E(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_E);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_H(void) {
    LOG_DEBUG("OPC_LD_HL_H(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_H);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_L(void) {
    LOG_DEBUG("OPC_LD_HL_L(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_L);

    cpu.cycle_count += 8;
}

void OPC_LD_HL_A(void) {
    LOG_DEBUG("OPC_LD_HL_A(void)");
    mmu_write_byte(CPU_DREG_HL, CPU_REG_A);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_HL_d8(void) {
    LOG_DEBUG("OPC_LD_HL_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    mmu_write_byte(CPU_DREG_HL, immediate);
    ++cpu.PC;

    cpu.cycle_count += 12;
}

// LD A, n
void OPC_LD_A_B(void) {
    LOG_DEBUG("OPC_LD_A_B(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_B);

    cpu.cycle_count += 4;
}

void OPC_LD_A_C(void) {
    LOG_DEBUG("OPC_LD_A_C(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_C);

    cpu.cycle_count += 4;
}

void OPC_LD_A_D(void) {
    LOG_DEBUG("OPC_LD_A_D(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_D);

    cpu.cycle_count += 4;
}

void OPC_LD_A_E(void) {
    LOG_DEBUG("OPC_LD_A_E(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_E);

    cpu.cycle_count += 4;
}

void OPC_LD_A_H(void) {
    LOG_DEBUG("OPC_LD_A_H(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_H);

    cpu.cycle_count += 4;
}

void OPC_LD_A_L(void) {
    LOG_DEBUG("OPC_LD_A_L(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_L);

    cpu.cycle_count += 4;
}

void OPC_LD_A_HL(void) {
    LOG_DEBUG("OPC_LD_A_HL(void)");
    LD_REG_REG(&CPU_REG_A, mmu_get_byte(CPU_DREG_HL));
    ++cpu.PC;

    cpu.cycle_count += 8;
}

void OPC_LD_A_A(void) {
    LOG_DEBUG("OPC_LD_A_A(void)");
    LD_REG_REG(&CPU_REG_A, CPU_REG_A);

    cpu.cycle_count += 4;
}

void OPC_LD_A_d8(void) {
    LOG_DEBUG("OPC_LD_A_d8(void)");
    LD_REG_d8(&CPU_REG_A);
    ++cpu.PC;

    cpu.cycle_count += 8;
}

/******************************************************
 *** 8-BIT ALU                                      ***
 ******************************************************/
static void ADD_A_n(uint8_t n) {
    uint8_t A            = CPU_REG_A;
    uint8_t A_nibble     = A & LO_NIBBLE_MASK;
    uint8_t n_nibble     = n & LO_NIBBLE_MASK;
    uint_fast16_t result = A + n;

    clear_flag_register(); // N is implicitly cleared
    set_flag(result > BYTE_MASK, C_FLAG);
    set_flag(A_nibble + n_nibble > LO_NIBBLE_MASK, H_FLAG);
    set_flag((result &= BYTE_MASK) == 0, Z_FLAG);

    CPU_REG_A = (uint8_t) result;
}

void OPC_ADD_A_A(void) {
    LOG_DEBUG("OPC_ADD_A_A(void)");
    ADD_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_B(void) {
    LOG_DEBUG("OPC_ADD_A_B(void)");
    ADD_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_C(void) {
    LOG_DEBUG("OPC_ADD_A_C(void)");
    ADD_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_D(void) {
    LOG_DEBUG("OPC_ADD_A_D(void)");
    ADD_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_E(void) {
    LOG_DEBUG("OPC_ADD_A_E(void)");
    ADD_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_H(void) {
    LOG_DEBUG("OPC_ADD_A_H(void)");
    ADD_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_L(void) {
    LOG_DEBUG("OPC_ADD_A_L(void)");
    ADD_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADD_A_HL(void) {
    LOG_DEBUG("OPC_ADD_A_HL(void)");
    ADD_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_ADD_A_d8(void) {
    LOG_DEBUG("OPC_ADD_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    ADD_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void ADC_A_n(uint8_t n) {
    uint8_t A            = CPU_REG_A;
    uint8_t A_nibble     = A & LO_NIBBLE_MASK;
    uint8_t n_nibble     = n & LO_NIBBLE_MASK;
    uint8_t c_flag       = get_flag(C_FLAG);
    uint_fast16_t result = (uint_fast16_t) (A + n + c_flag);

    clear_flag_register(); // N is implicitly cleared
    set_flag(result > BYTE_MASK, C_FLAG);
    set_flag(A_nibble + n_nibble + c_flag > LO_NIBBLE_MASK, H_FLAG);
    set_flag((result &= BYTE_MASK) == 0, Z_FLAG);

    CPU_REG_A = (uint8_t) result;
}

void OPC_ADC_A_A(void) {
    LOG_DEBUG("OPC_ADC_A_A(void)");
    ADC_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_B(void) {
    LOG_DEBUG("OPC_ADC_A_B(void)");
    ADC_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_C(void) {
    LOG_DEBUG("OPC_ADC_A_C(void)");
    ADC_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_D(void) {
    LOG_DEBUG("OPC_ADC_A_D(void)");
    ADC_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_E(void) {
    LOG_DEBUG("OPC_ADC_A_E(void)");
    ADC_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_H(void) {
    LOG_DEBUG("OPC_ADC_A_H(void)");
    ADC_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_L(void) {
    LOG_DEBUG("OPC_ADC_A_L(void)");
    ADC_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_ADC_A_HL(void) {
    LOG_DEBUG("OPC_ADC_A_HL(void)");
    ADC_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_ADC_A_d8(void) {
    LOG_DEBUG("OPC_ADC_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    ADC_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void SUB_A_n(uint8_t n) {
    uint8_t A      = CPU_REG_A;
    uint8_t result = A - n;

    clear_flag_register();
    set_flag(!result, Z_FLAG);
    set_flag(1, N_FLAG);
    set_flag((n & LO_NIBBLE_MASK) > (A & LO_NIBBLE_MASK), H_FLAG);
    set_flag(n > A, C_FLAG);

    CPU_REG_A = result;
}

void OPC_SUB_A_A(void) {
    LOG_DEBUG("OPC_SUB_A_A(void)");
    SUB_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_B(void) {
    LOG_DEBUG("OPC_SUB_A_B(void)");
    SUB_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_C(void) {
    LOG_DEBUG("OPC_SUB_A_C(void)");
    SUB_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_D(void) {
    LOG_DEBUG("OPC_SUB_A_D(void)");
    SUB_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_E(void) {
    LOG_DEBUG("OPC_SUB_A_E(void)");
    SUB_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_H(void) {
    LOG_DEBUG("OPC_SUB_A_H(void)");
    SUB_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_L(void) {
    LOG_DEBUG("OPC_SUB_A_L(void)");
    SUB_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SUB_A_HL(void) {
    LOG_DEBUG("OPC_SUB_A_HL(void)");
    SUB_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_SUB_A_d8(void) {
    LOG_DEBUG("OPC_SUB_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    SUB_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void SBC_A_n(uint8_t n) {
    uint8_t A      = CPU_REG_A;
    uint8_t carry  = get_flag(C_FLAG);
    uint8_t result = (uint8_t) (A - carry - n);

    clear_flag_register();
    set_flag(!result, Z_FLAG);
    set_flag(1, N_FLAG);
    set_flag(((n + carry) & LO_NIBBLE_MASK) > (A & LO_NIBBLE_MASK), H_FLAG);
    set_flag((n + carry) > A, C_FLAG);

    CPU_REG_A = result;
}

void OPC_SBC_A_A(void) {
    LOG_DEBUG("OPC_SBC_A_A(void)");
    SBC_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_B(void) {
    LOG_DEBUG("OPC_SBC_A_B(void)");
    SBC_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_C(void) {
    LOG_DEBUG("OPC_SBC_A_C(void)");
    SBC_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_D(void) {
    LOG_DEBUG("OPC_SBC_A_D(void)");
    SBC_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_E(void) {
    LOG_DEBUG("OPC_SBC_A_E(void)");
    SBC_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_H(void) {
    LOG_DEBUG("OPC_SBC_A_H(void)");
    SBC_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_L(void) {
    LOG_DEBUG("OPC_SBC_A_L(void)");
    SBC_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_SBC_A_HL(void) {
    LOG_DEBUG("OPC_SBC_A_HL(void)");
    SBC_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_SBC_A_d8(void) {
    LOG_DEBUG("OPC_SBC_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    SBC_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void AND_A_n(uint8_t n) {
    uint8_t result = CPU_REG_A & n;

    clear_flag_register(); // C and N are implicitly cleared
    set_flag(!result, Z_FLAG);
    set_flag(1, H_FLAG);

    CPU_REG_A = result;
}

void OPC_AND_A_A(void) {
    LOG_DEBUG("OPC_AND_A_A(void)");
    AND_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_B(void) {
    LOG_DEBUG("OPC_AND_A_B(void)");
    AND_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_C(void) {
    LOG_DEBUG("OPC_AND_A_C(void)");
    AND_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_D(void) {
    LOG_DEBUG("OPC_AND_A_D(void)");
    AND_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_E(void) {
    LOG_DEBUG("OPC_AND_A_E(void)");
    AND_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_H(void) {
    LOG_DEBUG("OPC_AND_A_H(void)");
    AND_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_L(void) {
    LOG_DEBUG("OPC_AND_A_L(void)");
    AND_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_AND_A_HL(void) {
    LOG_DEBUG("OPC_AND_A_HL(void)");
    AND_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_AND_A_d8(void) {
    LOG_DEBUG("OPC_AND_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    AND_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void OR_A_n(uint8_t n) {
    uint8_t result = CPU_REG_A | n;

    clear_flag_register(); // C, H, and N are implicitly cleared
    set_flag(!result, Z_FLAG);

    CPU_REG_A = result;
}

void OPC_OR_A_A(void) {
    LOG_DEBUG("OPC_OR_A_A(void)");
    OR_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_B(void) {
    LOG_DEBUG("OPC_OR_A_B(void)");
    OR_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_C(void) {
    LOG_DEBUG("OPC_OR_A_C(void)");
    OR_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_D(void) {
    LOG_DEBUG("OPC_OR_A_D(void)");
    OR_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_E(void) {
    LOG_DEBUG("OPC_OR_A_E(void)");
    OR_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_H(void) {
    LOG_DEBUG("OPC_OR_A_H(void)");
    OR_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_L(void) {
    LOG_DEBUG("OPC_OR_A_L(void)");
    OR_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_OR_A_HL(void) {
    LOG_DEBUG("OPC_OR_A_HL(void)");
    OR_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_OR_A_d8(void) {
    LOG_DEBUG("OPC_OR_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    OR_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void XOR_A_n(uint8_t n) {
    clear_flag_register();

    CPU_REG_A ^= n;
    set_flag((CPU_REG_A == 0x00), Z_FLAG);
}

void OPC_XOR_A_A(void) {
    LOG_DEBUG("OPC_XOR_A_A(void)");
    XOR_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_B(void) {
    LOG_DEBUG("OPC_XOR_A_B(void)");
    XOR_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_C(void) {
    LOG_DEBUG("OPC_XOR_A_C(void)");
    XOR_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_D(void) {
    LOG_DEBUG("OPC_XOR_A_D(void)");
    XOR_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_E(void) {
    LOG_DEBUG("OPC_XOR_A_E(void)");
    XOR_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_H(void) {
    LOG_DEBUG("OPC_XOR_A_H(void)");
    XOR_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_L(void) {
    LOG_DEBUG("OPC_XOR_A_L(void)");
    XOR_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_XOR_A_HL(void) {
    LOG_DEBUG("OPC_XOR_A_HL(void)");
    XOR_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_XOR_A_d8(void) {
    LOG_DEBUG("OPC_XOR_A_d8(void)");
    XOR_A_n(mmu_get_byte(cpu.PC + 1));
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void CP_A_n(uint8_t n) {
    uint8_t A      = CPU_REG_A;
    uint8_t result = A - n;

    clear_flag_register();
    set_flag(!result, Z_FLAG);
    set_flag(1, N_FLAG);
    set_flag((n & LO_NIBBLE_MASK) > (A & LO_NIBBLE_MASK), H_FLAG);
    set_flag(n > A, C_FLAG);
}

void OPC_CP_A_A(void) {
    LOG_DEBUG("OPC_CP_A_A(void)");
    CP_A_n(CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_B(void) {
    LOG_DEBUG("OPC_CP_A_B(void)");
    CP_A_n(CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_C(void) {
    LOG_DEBUG("OPC_CP_A_C(void)");
    CP_A_n(CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_D(void) {
    LOG_DEBUG("OPC_CP_A_D(void)");
    CP_A_n(CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_E(void) {
    LOG_DEBUG("OPC_CP_A_E(void)");
    CP_A_n(CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_H(void) {
    LOG_DEBUG("OPC_CP_A_H(void)");
    CP_A_n(CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_L(void) {
    LOG_DEBUG("OPC_CP_A_L(void)");
    CP_A_n(CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_CP_A_HL(void) {
    LOG_DEBUG("OPC_CP_A_HL(void)");
    CP_A_n(mmu_get_byte(CPU_DREG_HL));
    cpu.cycle_count += 8;
    ++cpu.PC;
}

void OPC_CP_A_d8(void) {
    LOG_DEBUG("OPC_CP_A_d8(void)");
    uint8_t immediate = mmu_get_byte(cpu.PC + 1);
    CP_A_n(immediate);
    cpu.cycle_count += 8;
    cpu.PC += 2;
}

static void INC_n(uint8_t *const reg) {
    ++(*reg);

    set_flag(!(*reg), Z_FLAG);
    set_flag(((*reg) & 0x10) == 0x10, H_FLAG);
    clear_flag(N_FLAG);
}

void OPC_INC_B(void) {
    LOG_DEBUG("OPC_INC_B(void)");
    INC_n(&CPU_REG_B);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_C(void) {
    LOG_DEBUG("OPC_INC_C(void)");
    INC_n(&CPU_REG_C);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_D(void) {
    LOG_DEBUG("OPC_INC_D(void)");
    INC_n(&CPU_REG_D);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_E(void) {
    LOG_DEBUG("OPC_INC_E(void)");
    INC_n(&CPU_REG_E);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_H(void) {
    LOG_DEBUG("OPC_INC_H(void)");
    INC_n(&CPU_REG_H);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_L(void) {
    LOG_DEBUG("OPC_INC_L(void)");
    INC_n(&CPU_REG_L);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_A(void) {
    LOG_DEBUG("OPC_INC_A(void)");
    INC_n(&CPU_REG_A);

    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_INC_HL(void) {
    LOG_DEBUG("OPC_INC_HL(void)");
    uint8_t value = mmu_get_byte(CPU_DREG_HL);
    INC_n(&value);
    mmu_write_byte(CPU_DREG_HL, value);

    cpu.cycle_count += 12;
    ++cpu.PC;
}

static void DEC_n(uint8_t *const addr) {
    // Simplifications made for half carry flag check:
    // (*addr & LO_NIBBLE_MASK) < (n & LO_NIBBLE_MASK) where n = 1
    // (*addr & LO_NIBBLE_MASK) < (1 & LO_NIBBLE_MASK)
    // (*addr & LO_NIBBLE_MASK) < 1
    // (*addr & LO_NIBBLE_MASK) == 0
    set_flag(!(*addr & LO_NIBBLE_MASK), H_FLAG);

    --(*addr);

    set_flag(!(*addr), Z_FLAG);
    set_flag(1, N_FLAG);

    // Flag C is not affected
}

void OPC_DEC_A(void) {
    LOG_DEBUG("OPC_DEC_A(void)");
    DEC_n(&CPU_REG_A);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_B(void) {
    LOG_DEBUG("OPC_DEC_B(void)");
    DEC_n(&CPU_REG_B);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_C(void) {
    LOG_DEBUG("OPC_DEC_C(void)");
    DEC_n(&CPU_REG_C);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_D(void) {
    LOG_DEBUG("OPC_DEC_D(void)");
    DEC_n(&CPU_REG_D);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_E(void) {
    LOG_DEBUG("OPC_DEC_E(void)");
    DEC_n(&CPU_REG_E);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_H(void) {
    LOG_DEBUG("OPC_DEC_H(void)");
    DEC_n(&CPU_REG_H);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_L(void) {
    LOG_DEBUG("OPC_DEC_L(void)");
    DEC_n(&CPU_REG_L);
    cpu.cycle_count += 4;
    ++cpu.PC;
}

void OPC_DEC_HL(void) {
    LOG_DEBUG("OPC_DEC_HL(void)");
    uint8_t value = mmu_get_byte(CPU_DREG_HL);
    DEC_n(&value);
    mmu_write_byte(CPU_DREG_HL, value);
    cpu.cycle_count += 12;
    ++cpu.PC;
}

void OPC_RST_x(uint16_t address) {
    mmu_stack_push(cpu.PC);
    cpu.PC = mmu_get_two_bytes(address);
    cpu.cycle_count += 16;
}

void OPC_RST_00(void) {
    LOG_DEBUG("OPC_RST_00");
    OPC_RST_x(0x0000);
}
void OPC_RST_10(void) {
    LOG_DEBUG("OPC_RST_10");
    OPC_RST_x(0x0010);
}
void OPC_RST_20(void) {
    LOG_DEBUG("OPC_RST_20");
    OPC_RST_x(0x0020);
}
void OPC_RST_30(void) {
    LOG_DEBUG("OPC_RST_30");
    OPC_RST_x(0x0030);
}
void OPC_RST_08(void) {
    LOG_DEBUG("OPC_RST_08");
    OPC_RST_x(0x0008);
}
void OPC_RST_18(void) {
    LOG_DEBUG("OPC_RST_18");
    OPC_RST_x(0x0018);
}
void OPC_RST_28(void) {
    LOG_DEBUG("OPC_RST_28");
    OPC_RST_x(0x0028);
}
void OPC_RST_38(void) {
    LOG_DEBUG("OPC_RST_38");
    OPC_RST_x(0x0038);
}
void OPC_LD_xx_u16(uint16_t load_addr) {
    mmu_write_two_bytes(load_addr, mmu_get_two_bytes(cpu.PC));
    cpu.PC += 2;

    cpu.cycle_count += 12;
}

void OPC_LD_BC_u16(void) {
    LOG_DEBUG("OPC_LD_BC_u16");
    OPC_LD_xx_u16(CPU_DREG_BC);
}

void OPC_LD_DE_u16(void) {
    LOG_DEBUG("OPC_LD_DE_u16");
    OPC_LD_xx_u16(CPU_DREG_DE);
}

void OPC_LD_HL_u16(void) {
    LOG_DEBUG("OPC_LD_HL_u16");
    OPC_LD_xx_u16(CPU_DREG_HL);
}

void OPC_LD_SP_u16(void) {
    LOG_DEBUG("OPC_LD_SP_u16");
    OPC_LD_xx_u16(cpu.SP);
}

void OPC_JR_i8(void) {
    LOG_DEBUG("OPC_JR_i8");
    int8_t n = (int8_t) mmu_get_byte(cpu.PC);
    cpu.PC++;

    LOG_DEBUG("Jumping to PC (0x%02x) + 0x%02x", cpu.PC, n);
    cpu.PC = (uint16_t) (cpu.PC + n);
    cpu.cycle_count += 12;
}

void CALL(uint16_t address);
void CALL(uint16_t address) {
    LOG_DEBUG("CALL");
    mmu_stack_push(cpu.PC);
    cpu.PC = address;
}

void OPC_CALL_u16(void) {
    LOG_DEBUG("OPC_CALL_u16");
    uint16_t nn = mmu_get_two_bytes(cpu.PC);
    CALL(nn);
}
void OPC_CALL_cc_u16(uint8_t bit, uint8_t branching_condition);
void OPC_CALL_cc_u16(uint8_t bit, uint8_t branching_condition) {
    LOG_DEBUG("OPC_CALL_cc_u16");
    uint16_t nn = mmu_get_two_bytes(cpu.PC);

    if (bit == branching_condition) {
        CALL(nn);
        cpu.cycle_count += 24;
    } else {
        cpu.cycle_count += 12;
    }
}

void OPC_CALL_Z_u16(void) {
    LOG_DEBUG("OPC_CALL_Z_u16");
    OPC_CALL_cc_u16(get_flag(Z_FLAG), 1);
}

void OPC_CALL_NZ_u16(void) {
    LOG_DEBUG("OPC_CALL_NZ_u16");
    OPC_CALL_cc_u16(get_flag(Z_FLAG), 0);
}

void OPC_CALL_C_u16(void) {
    LOG_DEBUG("OPC_CALL_C_u16");
    OPC_CALL_cc_u16(get_flag(C_FLAG), 1);
}

void OPC_CALL_NC_u16(void) {
    LOG_DEBUG("OPC_CALL_NC_u16");
    OPC_CALL_cc_u16(get_flag(C_FLAG), 0);
}

void OPC_DI(void) {
    LOG_DEBUG("OPC_DI(void)");
    // TODO: implement interrupts
}

// ----------------------- Misc / control
void OPC_STOP(void) {
    LOG_DEBUG("OPC_STOP");
    // STOP is encoded as the two-byte sequence 0x10 0x00; consume the second byte.
    ++cpu.PC;
    cpu.cycle_count += 4;
    // TODO: add STOP semantics (CPU + LCD halt until button input)
}

void OPC_HALT(void) {
    LOG_DEBUG("OPC_HALT");
    cpu.cycle_count += 4;
    // TODO: halt CPU until next interrupt
}

void OPC_EI(void) {
    LOG_DEBUG("OPC_EI");
    cpu.cycle_count += 4;
    // TODO: enable interrupts
}

void OPC_DAA(void) {
    LOG_DEBUG("OPC_DAA");
    uint16_t a      = CPU_REG_A;
    uint8_t  carry  = 0;
    uint16_t adjust = 0;

    if (get_flag(N_FLAG)) {
        if (get_flag(H_FLAG)) {
            adjust |= 0x06;
        }
        if (get_flag(C_FLAG)) {
            adjust |= 0x60;
            carry = 1;
        }
        a = (uint16_t) (a - adjust);
    } else {
        if (get_flag(H_FLAG) || (a & LO_NIBBLE_MASK) > 0x09) {
            adjust |= 0x06;
        }
        if (get_flag(C_FLAG) || a > 0x99) {
            adjust |= 0x60;
            carry = 1;
        }
        a = (uint16_t) (a + adjust);
    }

    CPU_REG_A = (uint8_t) (a & BYTE_MASK);

    clear_flag(Z_FLAG);
    set_flag(CPU_REG_A == 0, Z_FLAG);
    clear_flag(H_FLAG);
    clear_flag(C_FLAG);
    set_flag(carry, C_FLAG);

    cpu.cycle_count += 4;
}

void OPC_CPL(void) {
    LOG_DEBUG("OPC_CPL");
    CPU_REG_A = (uint8_t) (~CPU_REG_A);
    set_flag(1, N_FLAG);
    set_flag(1, H_FLAG);
    cpu.cycle_count += 4;
}

void OPC_SCF(void) {
    LOG_DEBUG("OPC_SCF");
    clear_flag(N_FLAG);
    clear_flag(H_FLAG);
    set_flag(1, C_FLAG);
    cpu.cycle_count += 4;
}

void OPC_CCF(void) {
    LOG_DEBUG("OPC_CCF");
    uint8_t old_c = get_flag(C_FLAG);
    clear_flag(N_FLAG);
    clear_flag(H_FLAG);
    clear_flag(C_FLAG);
    set_flag(!old_c, C_FLAG);
    cpu.cycle_count += 4;
}


// ----------------------- Rotates on A
void OPC_RLCA(void) {
    LOG_DEBUG("OPC_RLCA");
    uint8_t bit7 = (uint8_t) ((CPU_REG_A >> 7) & 1);
    CPU_REG_A    = (uint8_t) ((CPU_REG_A << 1) | bit7);
    clear_flag_register();
    set_flag(bit7, C_FLAG);
    cpu.cycle_count += 4;
}

void OPC_RRCA(void) {
    LOG_DEBUG("OPC_RRCA");
    uint8_t bit0 = (uint8_t) (CPU_REG_A & 1);
    CPU_REG_A    = (uint8_t) ((CPU_REG_A >> 1) | (bit0 << 7));
    clear_flag_register();
    set_flag(bit0, C_FLAG);
    cpu.cycle_count += 4;
}

void OPC_RLA(void) {
    LOG_DEBUG("OPC_RLA");
    uint8_t old_carry = get_flag(C_FLAG);
    uint8_t bit7      = (uint8_t) ((CPU_REG_A >> 7) & 1);
    CPU_REG_A         = (uint8_t) ((CPU_REG_A << 1) | old_carry);
    clear_flag_register();
    set_flag(bit7, C_FLAG);
    cpu.cycle_count += 4;
}

void OPC_RRA(void) {
    LOG_DEBUG("OPC_RRA");
    uint8_t old_carry = get_flag(C_FLAG);
    uint8_t bit0      = (uint8_t) (CPU_REG_A & 1);
    CPU_REG_A         = (uint8_t) ((CPU_REG_A >> 1) | (old_carry << 7));
    clear_flag_register();
    set_flag(bit0, C_FLAG);
    cpu.cycle_count += 4;
}

// ----------------------- 16-BIT Loads
void OPC_LD_a16_SP(void) {
    LOG_DEBUG("OPC_LD_a16_SP");
    uint16_t addr = mmu_get_two_bytes(cpu.PC);
    mmu_write_two_bytes(addr, cpu.SP);
    cpu.PC += 2;
    cpu.cycle_count += 20;
}

void OPC_LD_SP_HL(void) {
    LOG_DEBUG("OPC_LD_SP_HL");
    cpu.SP = CPU_DREG_HL;
    cpu.cycle_count += 8;
}

void OPC_LD_HL_SP_i8(void) {
    LOG_DEBUG("OPC_LD_HL_SP_i8");
    int8_t   n  = (int8_t) mmu_get_byte(cpu.PC);
    uint16_t sp = cpu.SP;
    uint8_t  un = (uint8_t) n;

    clear_flag_register();
    set_flag(((sp & LO_NIBBLE_MASK) + (un & LO_NIBBLE_MASK)) > LO_NIBBLE_MASK, H_FLAG);
    set_flag(((sp & BYTE_MASK) + (un & BYTE_MASK)) > BYTE_MASK, C_FLAG);

    CPU_DREG_HL = (uint16_t) (sp + n);
    ++cpu.PC;
    cpu.cycle_count += 12;
}

// ----------------------- 16-BIT ALU
void OPC_INC_DE(void) {
    LOG_DEBUG("OPC_INC_DE");
    ++CPU_DREG_DE;
    cpu.cycle_count += 8;
}

void OPC_INC_HL16(void) {
    LOG_DEBUG("OPC_INC_HL16");
    ++CPU_DREG_HL;
    cpu.cycle_count += 8;
}

void OPC_INC_SP(void) {
    LOG_DEBUG("OPC_INC_SP");
    ++cpu.SP;
    cpu.cycle_count += 8;
}

void OPC_DEC_BC(void) {
    LOG_DEBUG("OPC_DEC_BC");
    --CPU_DREG_BC;
    cpu.cycle_count += 8;
}

void OPC_DEC_DE(void) {
    LOG_DEBUG("OPC_DEC_DE");
    --CPU_DREG_DE;
    cpu.cycle_count += 8;
}

void OPC_DEC_HL16(void) {
    LOG_DEBUG("OPC_DEC_HL16");
    --CPU_DREG_HL;
    cpu.cycle_count += 8;
}

void OPC_DEC_SP(void) {
    LOG_DEBUG("OPC_DEC_SP");
    --cpu.SP;
    cpu.cycle_count += 8;
}

static void ADD_HL_n(uint16_t n) {
    uint_fast32_t result = (uint_fast32_t) CPU_DREG_HL + n;

    clear_flag(N_FLAG);
    clear_flag(H_FLAG);
    clear_flag(C_FLAG);
    set_flag(((CPU_DREG_HL & 0x0FFF) + (n & 0x0FFF)) > 0x0FFF, H_FLAG);
    set_flag(result > 0xFFFF, C_FLAG);

    CPU_DREG_HL = (uint16_t) result;
}

void OPC_ADD_HL_BC(void) {
    LOG_DEBUG("OPC_ADD_HL_BC");
    ADD_HL_n(CPU_DREG_BC);
    cpu.cycle_count += 8;
}

void OPC_ADD_HL_DE(void) {
    LOG_DEBUG("OPC_ADD_HL_DE");
    ADD_HL_n(CPU_DREG_DE);
    cpu.cycle_count += 8;
}

void OPC_ADD_HL_HL(void) {
    LOG_DEBUG("OPC_ADD_HL_HL");
    ADD_HL_n(CPU_DREG_HL);
    cpu.cycle_count += 8;
}

void OPC_ADD_HL_SP(void) {
    LOG_DEBUG("OPC_ADD_HL_SP");
    ADD_HL_n(cpu.SP);
    cpu.cycle_count += 8;
}

void OPC_ADD_SP_i8(void) {
    LOG_DEBUG("OPC_ADD_SP_i8");
    int8_t   n  = (int8_t) mmu_get_byte(cpu.PC);
    uint16_t sp = cpu.SP;
    uint8_t  un = (uint8_t) n;

    clear_flag_register();
    set_flag(((sp & LO_NIBBLE_MASK) + (un & LO_NIBBLE_MASK)) > LO_NIBBLE_MASK, H_FLAG);
    set_flag(((sp & BYTE_MASK) + (un & BYTE_MASK)) > BYTE_MASK, C_FLAG);

    cpu.SP = (uint16_t) (sp + n);
    ++cpu.PC;
    cpu.cycle_count += 16;
}

// ----------------------- Jumps
void OPC_JP_a16(void) {
    LOG_DEBUG("OPC_JP_a16");
    cpu.PC = mmu_get_two_bytes(cpu.PC);
    cpu.cycle_count += 16;
}

void OPC_JP_HL(void) {
    LOG_DEBUG("OPC_JP_HL");
    cpu.PC = CPU_DREG_HL;
    cpu.cycle_count += 4;
}

// ----------------------- Stack
void OPC_PUSH_BC(void) {
    LOG_DEBUG("OPC_PUSH_BC");
    mmu_stack_push(CPU_DREG_BC);
    cpu.cycle_count += 16;
}

void OPC_PUSH_DE(void) {
    LOG_DEBUG("OPC_PUSH_DE");
    mmu_stack_push(CPU_DREG_DE);
    cpu.cycle_count += 16;
}

void OPC_PUSH_HL(void) {
    LOG_DEBUG("OPC_PUSH_HL");
    mmu_stack_push(CPU_DREG_HL);
    cpu.cycle_count += 16;
}

void OPC_PUSH_AF(void) {
    LOG_DEBUG("OPC_PUSH_AF");
    mmu_stack_push(CPU_DREG_AF);
    cpu.cycle_count += 16;
}

void OPC_POP_BC(void) {
    LOG_DEBUG("OPC_POP_BC");
    CPU_DREG_BC = mmu_stack_pop();
    cpu.cycle_count += 12;
}

void OPC_POP_DE(void) {
    LOG_DEBUG("OPC_POP_DE");
    CPU_DREG_DE = mmu_stack_pop();
    cpu.cycle_count += 12;
}

void OPC_POP_HL(void) {
    LOG_DEBUG("OPC_POP_HL");
    CPU_DREG_HL = mmu_stack_pop();
    cpu.cycle_count += 12;
}

void OPC_POP_AF(void) {
    LOG_DEBUG("OPC_POP_AF");
    // The lower nibble of F is hardwired to zero on the SM83.
    CPU_DREG_AF = (uint16_t) (mmu_stack_pop() & 0xFFF0);
    cpu.cycle_count += 12;
}

// ----------------------- Returns
void OPC_RET(void) {
    LOG_DEBUG("OPC_RET");
    cpu.PC = mmu_stack_pop();
    cpu.cycle_count += 16;
}

void OPC_RETI(void) {
    LOG_DEBUG("OPC_RETI");
    cpu.PC = mmu_stack_pop();
    cpu.cycle_count += 16;
    // TODO: enable interrupts (mirror of EI but immediate)
}

static void OPC_RET_cc(uint8_t bit, uint8_t branching_condition) {
    if (bit == branching_condition) {
        cpu.PC = mmu_stack_pop();
        cpu.cycle_count += 20;
    } else {
        cpu.cycle_count += 8;
    }
}

void OPC_RET_NZ(void) {
    LOG_DEBUG("OPC_RET_NZ");
    OPC_RET_cc(get_flag(Z_FLAG), 0);
}

void OPC_RET_Z(void) {
    LOG_DEBUG("OPC_RET_Z");
    OPC_RET_cc(get_flag(Z_FLAG), 1);
}

void OPC_RET_NC(void) {
    LOG_DEBUG("OPC_RET_NC");
    OPC_RET_cc(get_flag(C_FLAG), 0);
}

void OPC_RET_C(void) {
    LOG_DEBUG("OPC_RET_C");
    OPC_RET_cc(get_flag(C_FLAG), 1);
}

// ----------------------- CB Prefix
static void RLC_n(uint8_t *reg) {
    uint8_t bit7 = (uint8_t) ((*reg >> 7) & 1);
    *reg         = (uint8_t) ((*reg << 1) | bit7);
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit7, C_FLAG);
}

static void RRC_n(uint8_t *reg) {
    uint8_t bit0 = (uint8_t) (*reg & 1);
    *reg         = (uint8_t) ((*reg >> 1) | (bit0 << 7));
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit0, C_FLAG);
}

static void RL_n(uint8_t *reg) {
    uint8_t old_carry = get_flag(C_FLAG);
    uint8_t bit7      = (uint8_t) ((*reg >> 7) & 1);
    *reg              = (uint8_t) ((*reg << 1) | old_carry);
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit7, C_FLAG);
}

static void RR_n(uint8_t *reg) {
    uint8_t old_carry = get_flag(C_FLAG);
    uint8_t bit0      = (uint8_t) (*reg & 1);
    *reg              = (uint8_t) ((*reg >> 1) | (old_carry << 7));
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit0, C_FLAG);
}

static void SLA_n(uint8_t *reg) {
    uint8_t bit7 = (uint8_t) ((*reg >> 7) & 1);
    *reg         = (uint8_t) (*reg << 1);
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit7, C_FLAG);
}

static void SRA_n(uint8_t *reg) {
    uint8_t bit0 = (uint8_t) (*reg & 1);
    uint8_t bit7 = (uint8_t) (*reg & 0x80);
    *reg         = (uint8_t) ((*reg >> 1) | bit7);
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit0, C_FLAG);
}

static void SWAP_n(uint8_t *reg) {
    *reg = (uint8_t) (((*reg & LO_NIBBLE_MASK) << 4) | ((*reg & HI_NIBBLE_MASK) >> 4));
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
}

static void SRL_n(uint8_t *reg) {
    uint8_t bit0 = (uint8_t) (*reg & 1);
    *reg         = (uint8_t) (*reg >> 1);
    clear_flag_register();
    set_flag(*reg == 0, Z_FLAG);
    set_flag(bit0, C_FLAG);
}

static void BIT_n(uint8_t value, uint8_t bit) {
    uint8_t mask = (uint8_t) (1u << bit);
    clear_flag(Z_FLAG);
    set_flag((value & mask) == 0, Z_FLAG);
    clear_flag(N_FLAG);
    set_flag(1, H_FLAG);
    // C unaffected
}

static void RES_n(uint8_t *reg, uint8_t bit) {
    *reg = (uint8_t) (*reg & ~(1u << bit));
}

static void SET_n(uint8_t *reg, uint8_t bit) {
    *reg = (uint8_t) (*reg | (1u << bit));
}

// Wrappers: one void(void) function per CB opcode
#define CB_REG_OP(OP, REG)                          \
    static void OPC_CB_##OP##_##REG(void) {         \
        LOG_DEBUG("OPC_CB_" #OP "_" #REG);          \
        OP##_n(&CPU_REG_##REG);                     \
        cpu.cycle_count += 8;                       \
    }

#define CB_HL_OP(OP)                                \
    static void OPC_CB_##OP##_HL(void) {            \
        LOG_DEBUG("OPC_CB_" #OP "_HL");             \
        uint8_t value = mmu_get_byte(CPU_DREG_HL);  \
        OP##_n(&value);                             \
        mmu_write_byte(CPU_DREG_HL, value);         \
        cpu.cycle_count += 16;                      \
    }

#define CB_OP_ALL(OP)                               \
    CB_REG_OP(OP, B)                                \
    CB_REG_OP(OP, C)                                \
    CB_REG_OP(OP, D)                                \
    CB_REG_OP(OP, E)                                \
    CB_REG_OP(OP, H)                                \
    CB_REG_OP(OP, L)                                \
    CB_HL_OP(OP)                                    \
    CB_REG_OP(OP, A)

CB_OP_ALL(RLC)
CB_OP_ALL(RRC)
CB_OP_ALL(RL)
CB_OP_ALL(RR)
CB_OP_ALL(SLA)
CB_OP_ALL(SRA)
CB_OP_ALL(SWAP)
CB_OP_ALL(SRL)

#define CB_BIT_REG(BIT, REG)                                \
    static void OPC_CB_BIT_##BIT##_##REG(void) {            \
        LOG_DEBUG("OPC_CB_BIT_" #BIT "_" #REG);             \
        BIT_n(CPU_REG_##REG, BIT);                          \
        cpu.cycle_count += 8;                               \
    }

#define CB_BIT_HL(BIT)                                      \
    static void OPC_CB_BIT_##BIT##_HL(void) {               \
        LOG_DEBUG("OPC_CB_BIT_" #BIT "_HL");                \
        BIT_n(mmu_get_byte(CPU_DREG_HL), BIT);              \
        cpu.cycle_count += 12;                              \
    }

#define CB_BIT_ALL(BIT)                                     \
    CB_BIT_REG(BIT, B)                                      \
    CB_BIT_REG(BIT, C)                                      \
    CB_BIT_REG(BIT, D)                                      \
    CB_BIT_REG(BIT, E)                                      \
    CB_BIT_REG(BIT, H)                                      \
    CB_BIT_REG(BIT, L)                                      \
    CB_BIT_HL(BIT)                                          \
    CB_BIT_REG(BIT, A)

CB_BIT_ALL(0)
CB_BIT_ALL(1)
CB_BIT_ALL(2)
CB_BIT_ALL(3)
CB_BIT_ALL(4)
CB_BIT_ALL(5)
CB_BIT_ALL(6)
CB_BIT_ALL(7)

#define CB_RW_REG(OP, BIT, REG)                             \
    static void OPC_CB_##OP##_##BIT##_##REG(void) {         \
        LOG_DEBUG("OPC_CB_" #OP "_" #BIT "_" #REG);         \
        OP##_n(&CPU_REG_##REG, BIT);                        \
        cpu.cycle_count += 8;                               \
    }

#define CB_RW_HL(OP, BIT)                                   \
    static void OPC_CB_##OP##_##BIT##_HL(void) {            \
        LOG_DEBUG("OPC_CB_" #OP "_" #BIT "_HL");            \
        uint8_t value = mmu_get_byte(CPU_DREG_HL);          \
        OP##_n(&value, BIT);                                \
        mmu_write_byte(CPU_DREG_HL, value);                 \
        cpu.cycle_count += 16;                              \
    }

#define CB_RW_ALL(OP, BIT)                                  \
    CB_RW_REG(OP, BIT, B)                                   \
    CB_RW_REG(OP, BIT, C)                                   \
    CB_RW_REG(OP, BIT, D)                                   \
    CB_RW_REG(OP, BIT, E)                                   \
    CB_RW_REG(OP, BIT, H)                                   \
    CB_RW_REG(OP, BIT, L)                                   \
    CB_RW_HL(OP, BIT)                                       \
    CB_RW_REG(OP, BIT, A)

CB_RW_ALL(RES, 0)
CB_RW_ALL(RES, 1)
CB_RW_ALL(RES, 2)
CB_RW_ALL(RES, 3)
CB_RW_ALL(RES, 4)
CB_RW_ALL(RES, 5)
CB_RW_ALL(RES, 6)
CB_RW_ALL(RES, 7)

CB_RW_ALL(SET, 0)
CB_RW_ALL(SET, 1)
CB_RW_ALL(SET, 2)
CB_RW_ALL(SET, 3)
CB_RW_ALL(SET, 4)
CB_RW_ALL(SET, 5)
CB_RW_ALL(SET, 6)
CB_RW_ALL(SET, 7)

#define CB_REGISTER_OP(BASE, OP)                            \
    do {                                                    \
        cb_prefixed_lookup[(BASE) + 0] = OPC_CB_##OP##_B;   \
        cb_prefixed_lookup[(BASE) + 1] = OPC_CB_##OP##_C;   \
        cb_prefixed_lookup[(BASE) + 2] = OPC_CB_##OP##_D;   \
        cb_prefixed_lookup[(BASE) + 3] = OPC_CB_##OP##_E;   \
        cb_prefixed_lookup[(BASE) + 4] = OPC_CB_##OP##_H;   \
        cb_prefixed_lookup[(BASE) + 5] = OPC_CB_##OP##_L;   \
        cb_prefixed_lookup[(BASE) + 6] = OPC_CB_##OP##_HL;  \
        cb_prefixed_lookup[(BASE) + 7] = OPC_CB_##OP##_A;   \
    } while (0)

#define CB_REGISTER_BITTED(BASE, OP, BIT)                          \
    do {                                                           \
        cb_prefixed_lookup[(BASE) + 0] = OPC_CB_##OP##_##BIT##_B;  \
        cb_prefixed_lookup[(BASE) + 1] = OPC_CB_##OP##_##BIT##_C;  \
        cb_prefixed_lookup[(BASE) + 2] = OPC_CB_##OP##_##BIT##_D;  \
        cb_prefixed_lookup[(BASE) + 3] = OPC_CB_##OP##_##BIT##_E;  \
        cb_prefixed_lookup[(BASE) + 4] = OPC_CB_##OP##_##BIT##_H;  \
        cb_prefixed_lookup[(BASE) + 5] = OPC_CB_##OP##_##BIT##_L;  \
        cb_prefixed_lookup[(BASE) + 6] = OPC_CB_##OP##_##BIT##_HL; \
        cb_prefixed_lookup[(BASE) + 7] = OPC_CB_##OP##_##BIT##_A;  \
    } while (0)

static void cb_optable_init(void) {
    CB_REGISTER_OP(0x00, RLC);
    CB_REGISTER_OP(0x08, RRC);
    CB_REGISTER_OP(0x10, RL);
    CB_REGISTER_OP(0x18, RR);
    CB_REGISTER_OP(0x20, SLA);
    CB_REGISTER_OP(0x28, SRA);
    CB_REGISTER_OP(0x30, SWAP);
    CB_REGISTER_OP(0x38, SRL);

    CB_REGISTER_BITTED(0x40, BIT, 0);
    CB_REGISTER_BITTED(0x48, BIT, 1);
    CB_REGISTER_BITTED(0x50, BIT, 2);
    CB_REGISTER_BITTED(0x58, BIT, 3);
    CB_REGISTER_BITTED(0x60, BIT, 4);
    CB_REGISTER_BITTED(0x68, BIT, 5);
    CB_REGISTER_BITTED(0x70, BIT, 6);
    CB_REGISTER_BITTED(0x78, BIT, 7);

    CB_REGISTER_BITTED(0x80, RES, 0);
    CB_REGISTER_BITTED(0x88, RES, 1);
    CB_REGISTER_BITTED(0x90, RES, 2);
    CB_REGISTER_BITTED(0x98, RES, 3);
    CB_REGISTER_BITTED(0xA0, RES, 4);
    CB_REGISTER_BITTED(0xA8, RES, 5);
    CB_REGISTER_BITTED(0xB0, RES, 6);
    CB_REGISTER_BITTED(0xB8, RES, 7);

    CB_REGISTER_BITTED(0xC0, SET, 0);
    CB_REGISTER_BITTED(0xC8, SET, 1);
    CB_REGISTER_BITTED(0xD0, SET, 2);
    CB_REGISTER_BITTED(0xD8, SET, 3);
    CB_REGISTER_BITTED(0xE0, SET, 4);
    CB_REGISTER_BITTED(0xE8, SET, 5);
    CB_REGISTER_BITTED(0xF0, SET, 6);
    CB_REGISTER_BITTED(0xF8, SET, 7);
}
