#ifndef YOBEMAG_CPU_H
#define YOBEMAG_CPU_H

#include <stdint.h>

/**
 * Encodes bit positions for flag register A
 */
typedef enum flag {
    /**
     * @brief Carry flag
     */
    C_FLAG = 4,
    /**
     * @brief Half carry flag
     */
    H_FLAG = 5,
    /**
     * @brief Subtract flag
     */
    N_FLAG = 6,
    /**
     * @brief Zero flag
     */
    Z_FLAG = 7,
} Flag;

typedef union DoubleWordReg {
    struct {
        uint8_t lo; // F, C, E, L
        uint8_t hi; // A, B, D, H
    } words;
    uint16_t dword;
} DoubleWordReg;

typedef struct CPU {
    DoubleWordReg HL;
    DoubleWordReg DE;
    DoubleWordReg BC;
    DoubleWordReg AF;

    uint16_t SP;
    uint16_t PC;

    uint16_t cycle_count;

    uint8_t opcode;
} CPU;

extern CPU cpu;

#define CPU_DREG_HL cpu.HL.dword
#define CPU_REG_H   cpu.HL.words.hi
#define CPU_REG_L   cpu.HL.words.lo

#define CPU_DREG_DE cpu.DE.dword
#define CPU_REG_D   cpu.DE.words.hi
#define CPU_REG_E   cpu.DE.words.lo

#define CPU_DREG_BC cpu.BC.dword
#define CPU_REG_B   cpu.BC.words.hi
#define CPU_REG_C   cpu.BC.words.lo

#define CPU_DREG_AF cpu.AF.dword
#define CPU_REG_A   cpu.AF.words.hi
#define CPU_REG_F   cpu.AF.words.lo

#define CPU_SP cpu.SP

void cpu_init(void);
void cpu_step(void);

void cpu_print_registers(void);

__attribute((always_inline)) inline uint8_t get_flag(Flag f) {
    return (CPU_REG_F >> f) & 1;
}

__attribute((always_inline)) inline void set_flag(uint8_t bit, Flag f) {
    CPU_REG_F |= bit << f;
}

__attribute((always_inline)) inline void clear_flag(Flag f) {
    CPU_REG_F &= (uint8_t) ~(1 << f);
}

__attribute__((always_inline)) inline void clear_flag_register(void) {
    CPU_REG_F = 0;
}

void REG_INC(uint8_t *reg);

void OPC_NOP(void);

void OPC_LD_BC(void);
void OPC_LD_BC_A(void);

void OPC_JP_NZ(void);
void OPC_JP_NC(void);
void OPC_JP_Z(void);
void OPC_JP_C(void);

void OPC_INC_BC(void);

/******************************************************
 *** Misc                                           ***
 ******************************************************/

/**
 * @brief If Zero Flag is not set, we add an immediate signed byte
 *        to the PC and jump to it.
 */
void OPC_JR_NZ(void);

/**
 * @brief If Carry Flag is not set, we add an immediate signed byte
 *        to the PC and jump to it.
 */
void OPC_JR_NC(void);

/**
 * @brief If Zero Flag is set, we add an immediate signed byte
 *        to the PC and jump to it.
 */
void OPC_JR_Z(void);

/**
 * @brief If Carry Flag is set, we add an immediate signed byte
 *        to the PC and jump to it.
 */
void OPC_JR_C(void);

/******************************************************
 *** 8-BIT Loads                                    ***
 ******************************************************/
void OPC_LD_SP(void);
void OPC_LD_DE_A(void);
void OPC_LD_HL_PLUS_A(void);
void OPC_LD_HL_MINUS_A(void);
void OPC_LD_FF00a8_A(void);
void OPC_LD_A_FF00a8(void);
void OPC_LD_FF00C_A(void);
void OPC_LD_A_FF00C(void);
void OPC_LD_a16_A(void);
void OPC_LD_A_a16(void);
void OPC_LD_A_BC(void);
void OPC_LD_A_DE(void);
void OPC_LD_A_HL_PLUS(void);
void OPC_LD_A_HL_MINUS(void);
void OPC_LD_B_B(void);
void OPC_LD_B_C(void);
void OPC_LD_B_D(void);
void OPC_LD_B_E(void);
void OPC_LD_B_H(void);
void OPC_LD_B_L(void);
void OPC_LD_B_HL(void);
void OPC_LD_B_A(void);
void OPC_LD_B_d8(void);
void OPC_LD_C_B(void);
void OPC_LD_C_C(void);
void OPC_LD_C_D(void);
void OPC_LD_C_E(void);
void OPC_LD_C_H(void);
void OPC_LD_C_L(void);
void OPC_LD_C_HL(void);
void OPC_LD_C_A(void);
void OPC_LD_C_d8(void);
void OPC_LD_D_B(void);
void OPC_LD_D_C(void);
void OPC_LD_D_D(void);
void OPC_LD_D_E(void);
void OPC_LD_D_H(void);
void OPC_LD_D_L(void);
void OPC_LD_D_HL(void);
void OPC_LD_D_A(void);
void OPC_LD_D_d8(void);
void OPC_LD_E_B(void);
void OPC_LD_E_C(void);
void OPC_LD_E_D(void);
void OPC_LD_E_E(void);
void OPC_LD_E_H(void);
void OPC_LD_E_L(void);
void OPC_LD_E_HL(void);
void OPC_LD_E_A(void);
void OPC_LD_E_d8(void);
void OPC_LD_H_B(void);
void OPC_LD_H_C(void);
void OPC_LD_H_D(void);
void OPC_LD_H_E(void);
void OPC_LD_H_H(void);
void OPC_LD_H_L(void);
void OPC_LD_H_HL(void);
void OPC_LD_H_A(void);
void OPC_LD_H_d8(void);
void OPC_LD_L_B(void);
void OPC_LD_L_C(void);
void OPC_LD_L_D(void);
void OPC_LD_L_E(void);
void OPC_LD_L_H(void);
void OPC_LD_L_L(void);
void OPC_LD_L_HL(void);
void OPC_LD_L_A(void);
void OPC_LD_L_d8(void);
void OPC_LD_HL_B(void);
void OPC_LD_HL_C(void);
void OPC_LD_HL_D(void);
void OPC_LD_HL_E(void);
void OPC_LD_HL_H(void);
void OPC_LD_HL_L(void);
void OPC_LD_HL_A(void);
void OPC_LD_HL_d8(void);
void OPC_LD_A_B(void);
void OPC_LD_A_C(void);
void OPC_LD_A_D(void);
void OPC_LD_A_E(void);
void OPC_LD_A_H(void);
void OPC_LD_A_L(void);
void OPC_LD_A_HL(void);
void OPC_LD_A_A(void);
void OPC_LD_A_d8(void);

uint16_t cpu_get_two_bytes(uint16_t addr);

/******************************************************
 *** 8-BIT ALU                                      ***
 ******************************************************/

// 8-bit ALU: ADD A,n
void OPC_ADD_A_A(void);

void OPC_ADD_A_B(void);

void OPC_ADD_A_C(void);

void OPC_ADD_A_D(void);

void OPC_ADD_A_E(void);

void OPC_ADD_A_H(void);

void OPC_ADD_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 	      then adds the fetched byte to A.
 *
 * @note This instruction requires the PC to be incremented by 1,
 *       but takes 8 CPU cycles because of fetching a byte from memory
 */
void OPC_ADD_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 	      then adds the fetched byte to A.
 *
 * @warning PC cannot be incremented before this operation is completed
 * 			since it reads the data from the opcode itself.
 *
 * @note This instruction requires the PC to be incremented by 2.
 */
void OPC_ADD_A_d8(void);

void OPC_ADC_A_A(void);

void OPC_ADC_A_B(void);

void OPC_ADC_A_C(void);

void OPC_ADC_A_D(void);

void OPC_ADC_A_E(void);

void OPC_ADC_A_H(void);

void OPC_ADC_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 		  then adds the fetched byte to A together with
 * 		  the carry flag.
 *
 * @note This instruction requires the PC to be incremented by 1,
 *       but takes 8 CPU cycles because of fetching a byte from memory.
 */
void OPC_ADC_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 		  then adds the fetched byte to A together with
 * 		  the carry flag.
 *
 * @warning PC cannot be incremented before this operation
 * 			is completed since it reads the data from the
 * 			opcode itself.
 *
 * @note This instruction requires the PC to be incremented by 2.
 */
void OPC_ADC_A_d8(void);

/**
 * @brief Subtract the value stored in Register A from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_A(void);

/**
 * @brief Subtract the value stored in Register B from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_B(void);

/**
 * @brief Subtract the value stored in Register C from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_C(void);

/**
 * @brief Subtract the value stored in Register D from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_D(void);

/**
 * @brief Subtract the value stored in Register E from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_E(void);

/**
 * @brief Subtract the value stored in Register H from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_H(void);

/**
 * @brief Subtract the value stored in Register L from
 * 	      the value stored in A and store it back into A.
 */
void OPC_SUB_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 	      then subtracts the fetched byte from A.
 *        As this operation fetches a byte from memory, this takes 8 cycles.
 */
void OPC_SUB_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 	      then subtracts the fetched byte from A.
 *
 * @warning PC cannot be incremented before this operation is completed
 * 			since it reads the data from the opcode itself.
 *          As this operation fetches a byte from memory, this takes 8 cycles.
 */
void OPC_SUB_A_d8(void);

/**
 * @brief Subtract the value stored in Register A from the value stored in A,
 * 	      additionally subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_A(void);

/**
 * @brief Subtract the value stored in Register B from the value stored in A,
 *        additionally subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_B(void);

/**
 * @brief Subtract the value stored in Register C from the value stored in A,
 *        additionally, subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_C(void);

/**
 * @brief Subtract the value stored in Register D from the value stored in A,
 *        additionally, subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_D(void);

/**
 * @brief Subtract the value stored in Register E from the value stored in A,
 *        additionally, subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_E(void);

/**
 * @brief Subtract the value stored in Register H from the value stored in A,
 *        additionally, subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_H(void);

/**
 * @brief Subtract the value stored in Register L from the value stored in A,
 *        additionally, subtract the carry flag, then store it in A.
 */
void OPC_SBC_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`, subtracts the fetched byte from A
 *        additionally, subtract the carry flag, then store it in A.
 *        As this operation fetches a byte from memory, this takes 8 cycles.
 */
void OPC_SBC_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1, subtracts the fetched byte from A
 *        additionally, subtract the carry flag, then store it in A.
 *
 * @warning PC cannot be incremented before this operation is completed
 * 			since it reads the data from the opcode itself.
 *          As this operation fetches a byte from memory, this takes 8 cycles.
 */
void OPC_SBC_A_d8(void);

void OPC_AND_A_A(void);

void OPC_AND_A_B(void);

void OPC_AND_A_C(void);

void OPC_AND_A_D(void);

void OPC_AND_A_E(void);

void OPC_AND_A_H(void);

void OPC_AND_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 		  then logical ands the fetched byte to A.
 *
 * @note This instruction requires the PC to be incremented by 1,
 *       but takes 8 CPU cycles because of fetching a byte from memory.
 */
void OPC_AND_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 		  then logical ands the fetched byte to A.
 *
 * @warning PC cannot be incremented before this operation
 * 			is completed since it reads the data from the
 * 			opcode itself.
 *
 * @note This instruction requires the PC to be incremented by 2.
 */
void OPC_AND_A_d8(void);

void OPC_OR_A_A(void);

void OPC_OR_A_B(void);

void OPC_OR_A_C(void);

void OPC_OR_A_D(void);

void OPC_OR_A_E(void);

void OPC_OR_A_H(void);

void OPC_OR_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 		  then logical ands the fetched byte to A.
 *
 * @note This instruction requires the PC to be incremented by 1,
 *       but takes 8 CPU cycles because of fetching a byte from memory.
 */
void OPC_OR_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 		  then logical ands the fetched byte to A.
 *
 * @warning PC cannot be incremented before this operation
 * 			is completed since it reads the data from the
 * 			opcode itself.
 *
 * @note This instruction requires the PC to be incremented by 2.
 */
void OPC_OR_A_d8(void);

/**
 * @brief XOR the value stored in Register A to the value stored in Register A.
 */
void OPC_XOR_A_A(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_B(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_C(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_D(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_E(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_H(void);

/**
 * @brief XOR the value stored in Register B with the value stored in Register A
 *        and store it in Register A.
 */
void OPC_XOR_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 *        then XORs the byte with the value stored in Register A
 * 	      and store it in Register A.
 *
 * @note Since this operation fetches a byte from memory, it takes 8 cycles.
 */
void OPC_XOR_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 	      then XORs the byte with the value stored in Register A
 * 	      and store it in Register A.
 *
 * @warning PC cannot be incremented before this operation is completed
 * 			since it reads the data from the opcode itself.
 *
 * @note Since this operation fetches a byte from memory, it takes 8 cycles.
 */
void OPC_XOR_A_d8(void);

/**
 * @brief Compare the value stored in Register A to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_A(void);

/**
 * @brief Compare the value stored in Register B to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_B(void);

/**
 * @brief Compare the value stored in Register C to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_C(void);

/**
 * @brief Compare the value stored in Register D to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_D(void);

/**
 * @brief Compare the value stored in Register E to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_E(void);

/**
 * @brief Compare the value stored in Register H to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_H(void);

/**
 * @brief Compare the value stored in Register L to the value stored in Register A.
 *
 * @note This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_L(void);

/**
 * @brief First fetches a byte from the address `HL`,
 * 	      then compares it to the value stored in Register A.
 *
 * @note Since this operation fetches a byte from memory, it takes 8 cycles.
 *       This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_HL(void);

/**
 * @brief First fetches an immediate byte from PC + 1,
 * 	      then compares it to the value stored in Register A.
 *
 * @warning PC cannot be incremented before this operation is completed
 * 			since it reads the data from the opcode itself.
 *
 * @note Since this operation fetches a byte from memory, it takes 8 cycles.
 *       This instruction only sets flags, but does not save the result into Register A.
 */
void OPC_CP_A_d8(void);

/**
 * @brief Increment the value stored in Register B.
 */
void OPC_INC_B(void);

/**
 * @brief Increment the value stored in Register C.
 */
void OPC_INC_C(void);

/**
 * @brief Increment the value stored in Register D.
 */
void OPC_INC_D(void);

/**
 * @brief Increment the value stored in Register E.
 */
void OPC_INC_E(void);

/**
 * @brief Increment the value stored in Register H.
 */
void OPC_INC_H(void);

/**
 * @brief Increment the value stored in Register L.
 */
void OPC_INC_L(void);

/**
 * @brief Increment the value stored at address `HL`.
 *
 * @note This instruction fetches a byte from address `HL`.
 *       Increments it and then writes the decremented byte
 *       back to the address `HL`. Hence it takes 12 cycles.
 */
void OPC_INC_HL(void);

/**
 * @brief Increment the value stored in Register A.
 */
void OPC_INC_A(void);

/**
 * @brief Decrement the value stored in Register A.
 */
void OPC_DEC_A(void);

/**
 * @brief Decrement the value stored in Register B.
 */
void OPC_DEC_B(void);

/**
 * @brief Decrement the value stored in Register C.
 */
void OPC_DEC_C(void);

/**
 * @brief Decrement the value stored in Register D.
 */
void OPC_DEC_D(void);

/**
 * @brief Decrement the value stored in Register E.
 */
void OPC_DEC_E(void);

/**
 * @brief Decrement the value stored in Register H.
 */
void OPC_DEC_H(void);

/**
 * @brief Decrement the value stored in Register L.
 */
void OPC_DEC_L(void);

/**
 * @brief Decrement the value stored at address `HL`.
 *
 * @note This instruction fetches a byte from address `HL`.
 *       Decrements it and then writes the decremented byte
 *       back to the address `HL`. Hence it takes 12 cycles.
 */
void OPC_DEC_HL(void);

/**
 * @brief Push PC onto the stack and load 0x0030 into PC.
 *
 */
void OPC_RST_30(void);

/**
 * @brief Push PC onto the stack and load 0x0000 into PC.
 *
 */
void OPC_RST_00(void);

/**
 * @brief Push PC onto the stack and load 0x0010 into PC.
 *
 */
void OPC_RST_10(void);

/**
 * @brief Push PC onto the stack and load 0x0020 into PC.
 *
 */
void OPC_RST_20(void);

/**
 * @brief Push PC onto the stack and load 0x0038 into PC.
 *
 */
void OPC_RST_38(void);

/**
 * @brief Push PC onto the stack and load 0x0008 into PC.
 *
 */
void OPC_RST_08(void);

/**
 * @brief Push PC onto the stack and load 0x0018 into PC.
 *
 */
void OPC_RST_18(void);

/**
 * @brief Push PC onto the stack and load 0x0028 into PC.
 *
 */
void OPC_RST_28(void);

/**
 * @brief Push PC onto the stack and load 0x0028 into PC.
 *
 */
void OPC_LD_BC_u16(void);
void OPC_LD_DE_u16(void);
void OPC_LD_HL_u16(void);
void OPC_LD_SP_u16(void);

/**
 * @brief Adds i8 to PC and jumps to it
 *
 */
void OPC_JR_i8(void);

void OPC_CALL_u16(void);
void OPC_CALL_Z_u16(void);
void OPC_CALL_NZ_u16(void);
void OPC_CALL_C_u16(void);
void OPC_CALL_NC_u16(void);

void OPC_DI(void);

/******************************************************
 *** Misc / control                                 ***
 ******************************************************/
void OPC_STOP(void);
void OPC_HALT(void);
void OPC_EI(void);
void OPC_DAA(void);
void OPC_CPL(void);
void OPC_SCF(void);
void OPC_CCF(void);

/******************************************************
 *** Rotates on A                                   ***
 ******************************************************/
void OPC_RLCA(void);
void OPC_RRCA(void);
void OPC_RLA(void);
void OPC_RRA(void);

/******************************************************
 *** 16-BIT Loads                                   ***
 ******************************************************/
void OPC_LD_a16_SP(void);
void OPC_LD_SP_HL(void);
void OPC_LD_HL_SP_i8(void);

/******************************************************
 *** 16-BIT ALU                                     ***
 ******************************************************/
void OPC_INC_DE(void);
void OPC_INC_HL16(void);
void OPC_INC_SP(void);

void OPC_DEC_BC(void);
void OPC_DEC_DE(void);
void OPC_DEC_HL16(void);
void OPC_DEC_SP(void);

void OPC_ADD_HL_BC(void);
void OPC_ADD_HL_DE(void);
void OPC_ADD_HL_HL(void);
void OPC_ADD_HL_SP(void);

void OPC_ADD_SP_i8(void);

/******************************************************
 *** Jumps                                          ***
 ******************************************************/
void OPC_JP_a16(void);
void OPC_JP_HL(void);

/******************************************************
 *** Stack                                          ***
 ******************************************************/
void OPC_PUSH_BC(void);
void OPC_PUSH_DE(void);
void OPC_PUSH_HL(void);
void OPC_PUSH_AF(void);

void OPC_POP_BC(void);
void OPC_POP_DE(void);
void OPC_POP_HL(void);
void OPC_POP_AF(void);

/******************************************************
 *** Returns                                        ***
 ******************************************************/
void OPC_RET(void);
void OPC_RETI(void);
void OPC_RET_NZ(void);
void OPC_RET_Z(void);
void OPC_RET_NC(void);
void OPC_RET_C(void);

#endif // YOBEMAG_CPU_H
