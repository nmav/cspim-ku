project(MIPS_APPS)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER mipsel-elf-gcc)
add_definitions(-Wall -O3 -mips1 -mno-check-zero-division -mlong-calls)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q -u __start")
include_directories(${MIPS_SOURCE_DIR}/libmips)

LINK_LIBRARIES(mips)

add_executable(cputorture cputorture.s)
set_property(SOURCE cputorture.s PROPERTY LANGUAGE C)

add_executable(hello hello.c)
add_executable(console console.c)
add_executable(file file.c)
add_executable(fact fact.c)
add_executable(adder adder.c)
add_executable(max3sum max3sum.c)
add_executable(digits digits.c)
add_executable(hanoi hanoi.c)

if(NOT HOSTED)
	include_directories(${MIPS_SOURCE_DIR}/vm ${MIPS_SOURCE_DIR}/vm/cspim ${MIPS_SOURCE_DIR}/libmips)
	add_executable(run-lvl1 run-lvl1.c)
	target_link_libraries(run-lvl1 mipsvm)
endif(NOT HOSTED)

