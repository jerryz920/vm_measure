cmake_minimum_required(VERSION 2.6)
project (VM_Measure)

set (CMAKE_C_FLAGS "-g -O2")
set (CMAKE_EXE_LINKER_FLAGS "-g -O2")
set (USE_DEBUG on)

option (X86 "whether this is a X86 compitable machine" ON)

include(CheckIncludeFiles)
check_include_files("stdint.h;inttypes.h" HAVE_C99_HEADERS)

if (NOT HAVE_C99_HEADERS)
  message(FATAL_ERROR "missing C99 headers. We need its feature.")
endif(NOT HAVE_C99_HEADERS)

check_include_files("unistd.h;time.h;sys/time.h" HAVE_POSIX_HEADER)
if (NOT HAVE_POSIX_HEADER)
  message(FATAL_ERROR "missing posix headers.")
endif (NOT HAVE_POSIX_HEADER)

check_include_files("sys/mman.h;sys/ipc.h;sys/shm.h" HAVE_IPC_HEADER)
if (NOT HAVE_IPC_HEADER)
  message(FATAL_ERROR "missing ipc headers.")
endif (NOT HAVE_IPC_HEADER)

include (CheckSymbolExists)
check_symbol_exists(SHM_HUGETLB "sys/ipc.h;sys/shm.h" HAVE_HUGE_TLB)

include (CheckFunctionExists)
#
#	Check sched exists
#
set (CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
set (CMAKE_REQUIRED_INCLUDES sched.h)
check_function_exists(sched_setaffinity HAVE_SCHED)
check_symbol_exists(CPU_ZERO "sched.h" HAVE_SCHED)
check_symbol_exists(CPU_SET "sched.h" HAVE_SCHED)
set (CMAKE_REQUIRED_DEFINITIONS)
set (CMAKE_REQUIRED_INCLUDES)


if (NOT HAVE_HUGE_TLB)
  message(FATAL_ERROR "missing huge tlb support.")
endif (NOT HAVE_HUGE_TLB)



#
#	Check cpuid exists. Actually we should write some
#	manually constructed program to test the CPUID
#	feature, and say if RDTSC is available. It should
#	on recent architectures, but just in case.
#
if(X86)
  set (HAVE_CPUID ON)
  set (HAVE_RDTSC ON)
endif(X86)


configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_SOURCE_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/")
add_subdirectory("${PROJECT_SOURCE_DIR}/misc")
link_directories("${PROJECT_SOURCE_DIR}/misc")

#add_executable(tlbsz tlbsz.c)
#target_link_libraries(tlbsz vmaux)

#add_executable(test_mykey test_mykey.c)
add_executable(tlbsz tlbsz.c)
add_executable(alloc alloc_policy.c)
add_executable(hugetlb huge_page.c)
#target_link_libraries(test_mykey vmaux m rt)
target_link_libraries(tlbsz vmaux m rt)
target_link_libraries(alloc vmaux m rt)
target_link_libraries(hugetlb vmaux m rt)
