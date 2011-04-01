cmake_minimum_required(VERSION 2.6)
PROJECT(MIPS)
ADD_DEFINITIONS(-g -Wall)

set(HOSTED ON CACHE BOOL "Hosted or embedded build.")

ADD_SUBDIRECTORY(vm)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(mipsapps)

if(HOSTED)
	ADD_SUBDIRECTORY(hostapps)
endif(HOSTED)
