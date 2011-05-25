project(NEW_MIPS_APPS)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER mipsel-openwrt-linux-gcc)
add_definitions(-Wall -O0 -mips1 -mno-check-zero-division -mlong-calls)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q -u __start")
include_directories(${MIPS_SOURCE_DIR}/libmips)

LINK_LIBRARIES(mips)

add_executable(test test.c)
add_executable(md5 main.c md5.c md5-compress.c)
