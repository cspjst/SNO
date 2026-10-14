# Copyright (c) 2026 Jeremy Simon Thornton, License: MIT
cmake_minimum_required(VERSION 3.10)

# Warning: This skips critical compiler checks.
# Only use this if Watcom fails CMake's detection
# Necessary to suppress compiler checks for cross compilation using OW2 and C under ARM environments
set(CMAKE_C_COMPILER_WORKS 1)

project(
    TEST
    VERSION 0.1.0
    LANGUAGES C
)

# Toolchain setup
set(CMAKE_SYSTEM_NAME DOS)      # Target DOS
set(CMAKE_C_COMPILER wcl)
set(CMAKE_CXX_COMPILER wcl)
set(CMAKE_LINKER wlink)         # Use Watcom's linker
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# watcom compiler options
# https://users.pja.edu.pl/~jms/qnx/help/watcom/compiler-tools/cpopts.html
if(WATCOM)
add_compile_options(
    # === REQUIRED (always on) ===
    -0                  # 8086/8088 instructions ONLY
    -bt=dos             # Target DOS
    #-l=dos              # Link DOS library

    # === MEMORY MODEL (pick ONE) ===
    -ml                 # Large model: far data pointers (flexible)
    # -zl               # Tiny model: .COM output, all segments unified (smallest)

    # === SIZE vs SPEED (pick ONE) ===
    -os                 # Optimize for CODE SIZE
    -ox                 # Enable extra optimizations (dead code elimination)
    -zp1                # Pack structs to 1-byte alignment (Critical for data size)
    -fpi87              # Assume 8087 exists (removes software FP emulation stubs if any are pulled in)
    # -ot               # Optimize for SPEED
    # -od               # Disable optimizations (debugging)

    # === DEBUG/INFO ===
    #-d0                # No debug info (smaller .obj + binary)
    #-d1                # Line numbers only
    -d2               # Full debug symbols

    # === STACK SAFETY ===
    #-e0                 # No stack checking (saves ~20 bytes)
    -e1               # Minimal stack overflow check

    # === LANGUAGE COMPAT ===
    -za99               # Partial C99 (va_arg, etc.)
    # (omit for pure C89, slightly smaller)

    # === WARNINGS ===
    #-w1                 # Mild warnings (avoids 8086 false positives)
    # -w0               # No warnings
     -w2               # More warnings

    # === BUILD OUTPUT ===
    #-zq                 # Quiet mode (cleaner output)
    # (omit for verbose build log)
)
add_definitions(
    -D__DOS__
    -D__8086__  # Explicit 8086 target
)

endif()

# WARNING: Using GLOB for convenience. If adding new files, rerun: ./cmk.sh
file(GLOB SOURCES
    CONFIGURE_DEPENDS
    *.c
    ../doslib/bioslib/src/BIOS/*.c
    ../doslib/src/DOS/*.c
    ENV/*.c
    LOG/*.c
    LIB/*.c
    MEM/*.c
    FXP/*.c
    FXP/VEC/*.c
    MDA/*.c
    CGA/*.c
    CGA/HI/*.c
    CGA/LO/*.c
    EGA/*.c
    HGA/*.c
    VGA/*.c
    SIM/*.c
    TEST/*.c
)

# message(Source list="${SOURCES}")

add_executable(TEST ${SOURCES})
