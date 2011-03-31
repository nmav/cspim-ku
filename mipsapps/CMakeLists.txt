project(MIPS_APPS)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER mipsel-elf-gcc)
add_definitions(-Wall -O3 -mips1 -mno-check-zero-division -mlong-calls)
#The -nostdlib option in my gcc causes _start to be used instead of __start
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-q,-e,_start")

add_executable(cputorture cputorture.s)
set_property(SOURCE cputorture.s PROPERTY LANGUAGE C)

add_executable(hello hello.c spim.c)
add_executable(console console.c spim.c)
add_executable(file file.c spim.c)
add_executable(fact fact.c spim.c)
add_executable(adder adder.c spim.c)
add_executable(max3sum max3sum.c spim.c)
add_executable(digits digits.c spim.c)
add_executable(hanoi hanoi.c spim.c)

if(NOT HOSTED)
	include_directories(${MIPS_SOURCE_DIR}/vm ${MIPS_SOURCE_DIR}/vm/cspim)
	add_executable(run-lvl1 run-lvl1.c spim.c)
	target_link_libraries(run-lvl1 mipsvm)
endif(NOT HOSTED)

