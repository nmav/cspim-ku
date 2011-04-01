project(NEW_MIPS_APPS)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER mipsel-elf-gcc)
add_definitions(-Wall -O3 -mips1 -mno-check-zero-division -mlong-calls)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q")
include_directories(${MIPS_SOURCE_DIR}/libmips)

add_executable(test test.c ../libmips/mips.c)

