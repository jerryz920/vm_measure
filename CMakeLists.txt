
cmake_minimum_required(VERSION 2.6)
project (VM_Measure)
option (SET_CACHE_LINE "Set the cache line size" ON)

if (SET_CACHE_LINE) 
  set (CACHE_LINE 64)
endif (SET_CACHE_LINE)

configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_SOURCE_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}/include")
add_subdirectory("${PROJECT_SOURCE_DIR}/misc")
link_directories("${PROJECT_SOURCE_DIR}/misc")

add_executable(test test.c)
target_link_libraries(test vmaux)

