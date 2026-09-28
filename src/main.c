#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "lcd.h"
#include "cpu.h"
#include "rom.h"
#include "mmu.h"
#include "cli.h"
#include "log.h"
#include "ppu.h"

void run_console(bool *halt);

int main(const int argc, char **const argv) {
    CLIArguments cli_args;
    cli_parse(&cli_args, argc, argv);

    log_set_lvl(cli_args.logging_level);
    atexit(log_teardown);

    rom_init(cli_args.rom_path);
    atexit(rom_destroy);
    LOG_INFO("Successfully initialized ROM");

    mmu_init();
    LOG_INFO("Successfully initialized MMU");

    lcd_init();
    atexit(lcd_teardown);
    LOG_INFO("Successfully initialized LCD");

    cpu_init();
    LOG_INFO("Successfully initialized CPU");

    ppu_init();
    LOG_INFO("Successfully initialized PPU");

    uint8_t iterations = 0;
    bool halt          = false;
    bool interactive   = false;
    while (!halt) {
        cpu_step();

        if (lcd_step())
            break;

        ++iterations;
        if (interactive) {
            run_console(&halt);
        }
    }
    LOG_INFO("Total number of iterations: %d", iterations);

    exit(EXIT_SUCCESS);
}

void run_console(bool *halt) {
    *halt = true;
    cpu_print_registers();

    char command[50];
    printf("> ");
    char *res = fgets(command, sizeof(command), stdin);
    if (res == NULL) {
        LOG_ERROR("ERROR: failed to get command");
        exit(-1);
    }
    command[strcspn(command, "\n")] = 0;
    printf("%02x\n", cpu.PC);
    printf("Command: %s\n", command);
}
