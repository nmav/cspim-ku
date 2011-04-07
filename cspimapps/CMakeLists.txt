project(NEW_MIPS_APPS)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER mipsel-elf-gcc)
add_definitions(-Wall -O0 -mips1 -mno-check-zero-division)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q")
include_directories(${MIPS_SOURCE_DIR}/libmips)

add_executable(test test.c ../libmips/mips.c)
add_executable(md5 main.c md5.c md5-compress.c ../libmips/mips.c)

