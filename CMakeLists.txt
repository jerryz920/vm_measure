
cmake_minimum_required(VERSION 2.6)
project (VM_Measure)

option (X86 "whether this is a X86 compitable machine" ON)

include (CheckFunctionExists) 
#
#	Check sched exists
#
set (CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
set (CMAKE_REQUIRED_INCLUDES sched.h)
check_function_exists(sched_setaffinity HAVE_SCHED)
set (CMAKE_REQUIRED_DEFINITIONS)
set (CMAKE_REQUIRED_INCLUDES)

#
#	Check cpuid exists
#
if(X86)
  set (HAVE_CPUID ON)
endif(X86)


configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_SOURCE_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}/include")
add_subdirectory("${PROJECT_SOURCE_DIR}/misc")
link_directories("${PROJECT_SOURCE_DIR}/misc")

add_executable(tlbsz tlbsz.c)
target_link_libraries(tlbsz vmaux)

