#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>
#include <criterion/redirect.h>
#include <libgen.h>

#include "cpu.h"
#include "mmu.h"
#include "rom.h"
#include "log.h"

#define MAX_PATH_LENGTH    (512)
#define MAX_LOG_MSG_LENGTH (512)

static const uint8_t partial_boot_rom[] = {0x31, 0xFE, 0xAF, 0x32, 0x0E, 0xE0, 0xF9, 0x06, 0x50};

Test(mmu, mmu_init_with_rom_load, .exit_code = EXIT_SUCCESS) {
    char *file_path_copy                = strdup(__FILE__);
    char rom_file_path[MAX_PATH_LENGTH] = {0};
    snprintf(rom_file_path, MAX_PATH_LENGTH, "%s/../roms/yobemag.gb", dirname(file_path_copy));
    rom_init(rom_file_path);
    mmu_init();
    mmu_set_boot_rom_active(true);

    for (int i = 0; i <= 8; ++i) {
        cr_assert(mmu_get_byte((uint16_t) ((1 << i) - 1)) == partial_boot_rom[i]);
    }

    mmu_set_boot_rom_active(false);
    free(file_path_copy);
}

Test(mmu, mmu_get_byte_boot_rom, .exit_code = EXIT_SUCCESS) {
    mmu_set_boot_rom_active(true);
    cr_assert(eq(u8, mmu_get_byte(113), 0x13));
    mmu_set_boot_rom_active(false);
}

Test(mmu, mmu_write_byte_outside_rom, .exit_code = EXIT_SUCCESS) {
    mmu_write_byte(ROM_LIMIT + 1, 0xFF);
    cr_assert(eq(u8, mmu_get_byte(ROM_LIMIT + 1), 0xFF));
}

Test(mmu, mmu_write_two_bytes_to_rom, .init = cr_redirect_stderr) {
    log_set_lvl(ERROR);

    mmu_write_two_bytes(ROM_LIMIT - 1, 0xAB);

    char buf[MAX_LOG_MSG_LENGTH] = {0};
    FILE *f_stderr               = cr_get_redirected_stderr();
    while (fread(buf, 1, sizeof(buf), f_stderr) > 0) {};
    fclose(f_stderr);

    cr_assert_not_null(strstr(buf, "reserved"));
    cr_assert_not_null(strstr(buf, "0x7fff"));
    cr_assert_not_null(strstr(buf, "ERROR"));
}

Test(mmu, mmu_write_two_bytes_outside_rom, .exit_code = EXIT_SUCCESS) {
    mmu_write_two_bytes(ROM_LIMIT, 0xFF);
    cr_assert(eq(u16, mmu_get_two_bytes(ROM_LIMIT), 0xFF));
}

Test(mmu, mmu_zero_clears_memory, .exit_code = EXIT_SUCCESS) {
    mmu_write_byte(ROM_LIMIT,     0xAB);
    mmu_write_byte(ROM_LIMIT + 1, 0xCD);

    mmu_zero();

    cr_assert(eq(u8, mmu_get_byte(ROM_LIMIT),     0x00));
    cr_assert(eq(u8, mmu_get_byte(ROM_LIMIT + 1), 0x00));
}

Test(mmu, mmu_stack_push_stores_bytes, .exit_code = EXIT_SUCCESS) {
    cpu_init();
    uint16_t sp_before = cpu.SP;

    mmu_stack_push(0xABCD);

    cr_assert(eq(u16, cpu.SP,                                        (uint16_t) (sp_before - 2)));
    cr_assert(eq(u8,  mmu_get_byte((uint16_t) (sp_before - 1)), 0xAB)); // high byte
    cr_assert(eq(u8,  mmu_get_byte((uint16_t) (sp_before - 2)), 0xCD)); // low byte
}

Test(mmu, mmu_stack_push_pop_roundtrip, .exit_code = EXIT_SUCCESS) {
    cpu_init();
    uint16_t sp_before = cpu.SP;

    mmu_stack_push(0x1234);
    uint16_t result = mmu_stack_pop();

    cr_assert(eq(u16, result,  0x1234));
    cr_assert(eq(u16, cpu.SP, sp_before));
}
