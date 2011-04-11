project(CSPIMLIB_MIPS)
LINK_LIBRARIES(mips)
set(CMAKE_C_COMPILER mipsel-elf-gcc)
add_definitions(-Wall -O3 -mips1 -mno-check-zero-division -mlong-calls)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q")
set(SOURCES mips.c start.s)

add_library(mips STATIC ${SOURCES})
set_property(SOURCE start.s PROPERTY LANGUAGE C)
