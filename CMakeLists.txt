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

add_library(akemi_usb SHARED "src/usb.cpp" "src/conv.cpp" "src/cp210x.cpp")
target_link_libraries(akemi_usb boost_thread)
target_link_libraries(akemi_usb usb-1.0)

add_executable(info "info.cpp")
target_link_libraries(info akemi_usb)

add_executable(akemi "akemi.cpp")
target_link_libraries(akemi akemi_usb)

add_executable(homura "homura.cpp" "src/serial_stream.cpp" "src/cp210x_stream.cpp" "src/dispatcher.cpp")
target_link_libraries(homura akemi_usb)
target_link_libraries(akemi_usb boost_system)

add_executable(madoka "madoka.cpp" "src/serial_stream.cpp" "src/serial_dmx.cpp")
target_link_libraries(madoka akemi_usb)
target_link_libraries(madoka boost_system boost_thread)