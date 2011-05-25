project(CSPIMLIB)
LINK_LIBRARIES(mipsvm)
INCLUDE_DIRECTORIES(${MIPS_SOURCE_DIR}/vm ${MIPS_SOURCE_DIR}/vm/hosted)
set(SOURCES util.c rc5-16.c)
set(SOURCES ${SOURCES} ../vm/cpuemu.c ../vm/opcodes.c ../vm/elfload.c ../vm/hosted/syscalls.c)

add_library(cspim STATIC ${SOURCES})

