cmake_minimum_required(VERSION 2.6)

project (akemi)

set (AKEMI_VERSION_MAJOR 0)
set (AKEMI_VERSION_MINOR 1)

set (PROJECT_SOURCE_DIR src)
set (PROJECT_INCLUDE_DIR inc)

configure_file (
	"${PROJECT_INCLUDE_DIR}/akemi_config.h.in"
	"${PROJECT_INCLUDE_DIR}/akemi_config.h"
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include_directories("${PROJECT_INCLUDE_DIR}")

file (GLOB AKEMI_LIB_SOURCES "${PROJECT_SOURCE_DIR}/*.cpp")

add_library(akemi_usb SHARED ${AKEMI_LIB_SOURCES})
target_link_libraries(akemi_usb boost_thread)
target_link_libraries(akemi_usb usb-1.0)

add_executable(info "info.cpp")
target_link_libraries(info akemi_usb)

add_executable(akemi "akemi.cpp")
target_link_libraries(akemi akemi_usb)

add_executable(homura "homura.cpp")
target_link_libraries(homura akemi_usb)
target_link_libraries(akemi_usb boost_system)