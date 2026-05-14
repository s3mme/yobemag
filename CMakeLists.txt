cmake_minimum_required(VERSION 3.0)
project(YOBEMAG C)

set(PRODUCT_NAME yobemag)
string(TOUPPER ${PRODUCT_NAME} UPPER_PRODUCT_NAME)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src)
set(SRC_FILES
    src/cpu.c
    src/main.c
    src/mmu.c
    src/lcd.c
    src/rom.c
    src/log.c
    src/cli.c)

add_executable(${PRODUCT_NAME} ${SRC_FILES})

message("[${UPPER_PRODUCT_NAME}] Using ${CMAKE_GENERATOR}")
message("[${UPPER_PRODUCT_NAME}] Using ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_VERSION}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DEBUG" OR CMAKE_BUILD_TYPE STREQUAL "debug")
    message("[${UPPER_PRODUCT_NAME}] Using -g")
    target_compile_options(${PRODUCT_NAME} PUBLIC -g)
    target_compile_definitions(${PRODUCT_NAME} PUBLIC YOBEMAG_DEBUG)
endif ()

target_compile_options(${PRODUCT_NAME} PUBLIC
                       # Standard GCC/Clang warnings
                       -Wall
                       -Wextra
                       -Werror
                       -Wunused
                       -Wshadow
                       -Wcast-align
                       -Wconversion
                       -Wdouble-promotion
                       -fstack-protector -Wstack-protector
                       -Wredundant-decls
                       -Wpacked
                       -Wmissing-declarations
                       -Wmissing-prototypes
                       -Wstrict-prototypes
                       -Wwrite-strings
                       -Wcast-qual
                       -Wundef
                       -Wfloat-equal
                       -Wstrict-overflow=4
                       -Wswitch-enum
                       -Wswitch-default
                       -Wmissing-include-dirs
                       -Wformat=2)

if (DEFINED OPTIMIZE)
    message("[${UPPER_PRODUCT_NAME}] Using -O${OPTIMIZE}")
    target_compile_options(${PRODUCT_NAME} PUBLIC -O${OPTIMIZE})
endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PRODUCT_NAME} PUBLIC
                           -Weverything
                           -Wno-declaration-after-statement
                           -Wno-padded
                           -Wno-gnu-designator
                           -ferror-limit=0)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PRODUCT_NAME} PUBLIC
                           # Standard GCC warnings: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
                           -Wlogical-op
                           -Wno-unknown-pragmas
                           -Warray-bounds=2
                           -Wjump-misses-init
                           -Wunsafe-loop-optimizations
                           -Wno-aggressive-loop-optimizations
                           -Wjump-misses-init
                           -Wtrampolines
                           -Wsuggest-attribute=cold
                           -Wsuggest-attribute=malloc
                           -Wsuggest-attribute=pure
                           -Wsuggest-attribute=const
                           -Wsuggest-attribute=noreturn
                           -Wsuggest-attribute=format
                           -fmax-errors=0)
endif ()

set_property(TARGET ${PRODUCT_NAME} PROPERTY C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

target_link_libraries(${PRODUCT_NAME} PUBLIC -lm -lSDL2 -pthread)

install(TARGETS ${PRODUCT_NAME} DESTINATION ~/.local/bin/ PERMISSIONS OWNER_EXECUTE)

################################################################################
###                                Unit Tests                                ###
################################################################################


if (${TEST})
    set(TEST_NAME ${PRODUCT_NAME}_test)

    set(TEST_FILES
        test/fixtures/cpu_mmu.c
        test/common/util.c
        test/common/alu.c
        test/8bit_lds_test.c
        test/alu_8bit_add_test.c
        test/alu_8bit_adc_test.c
        test/alu_8bit_and_test.c
        test/alu_8bit_sub_test.c
        test/alu_8bit_sbc_test.c
        test/alu_8bit_or_test.c
        test/alu_8bit_xor_test.c
        test/alu_8bit_cp_test.c
        test/alu_8bit_dec_test.c
        test/alu_8bit_inc_test.c
        test/cli_test.c
        test/rom_test.c
        test/mmu_test.c
        test/log_test.c
        test/jr_cc_n.c
        test/jp_cc_n.c
        test/json.c
        test/cpu_randomized.c
        test/cpu_randomized_test.c)

    set(TEST_SOURCES ${TEST_FILES} ${SRC_FILES})
    list(REMOVE_ITEM TEST_SOURCES "src/main.c")
    add_executable(${TEST_NAME} ${TEST_SOURCES})

    target_compile_definitions(${TEST_NAME} PUBLIC
        YOBEMAG_TEST
        CPU_RANDOMIZED_TEST_DIR="${CMAKE_SOURCE_DIR}/test/json")

    if (${COVERAGE})
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
            message("[${UPPER_PRODUCT_NAME}] Adding coverage flags")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 --coverage")
        else ()
            message(FATAL_ERROR "[${MSG_PREFIX}] GCC is required to output coverage reports")
        endif ()
    endif ()

    target_link_libraries(${TEST_NAME} PUBLIC -lm -lSDL2 -pthread -lcriterion)

    add_custom_target(test COMMAND ./${TEST_NAME} DEPENDS ${TEST_NAME})
endif ()

if (DEFINED SANITIZE)
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
        if (NOT ${SANITIZE} STREQUAL "valgrind")
            message("[${UPPER_PRODUCT_NAME}] Adding -fsanitize=${SANITIZE} and necessary compiler flags")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${SANITIZE}")

            if (${SANITIZE} STREQUAL "memory")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-memory-track-origins -fno-omit-frame-pointer -g -O2")
            else ()
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -fno-omit-frame-pointer -g")
            endif ()

            if (${TEST})
                add_custom_target(sanitize COMMAND ./${TEST_NAME} DEPENDS ${TEST_NAME})
            else ()
                add_custom_target(sanitize COMMAND ./${PRODUCT_NAME} DEPENDS ${PRODUCT_NAME})
            endif ()
        else ()
            message(FATAL_ERROR "[${UPPER_PRODUCT_NAME}] GCC is required to run valgrind")
        endif ()
    elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        if (${SANITIZE} STREQUAL "valgrind")
            message("[${UPPER_PRODUCT_NAME}] Adding no compile flags for ${SANITIZE}")

            if (${TEST})
                add_custom_target(sanitize COMMAND valgrind --leak-check=yes --trace-children=yes ./${TEST_NAME} DEPENDS ${TEST_NAME})
            else ()
                add_custom_target(sanitize COMMAND valgrind --leak-check=yes --trace-children=yes ./${PRODUCT_NAME} DEPENDS ${PRODUCT_NAME})
            endif ()
        else ()
            message(FATAL_ERROR "[${UPPER_PRODUCT_NAME}] clang is required to run address, memory, or undefined sanitizers")
        endif ()
    endif ()
endif ()
