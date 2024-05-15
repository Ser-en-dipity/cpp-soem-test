cmake_minimum_required(VERSION 3.8)

project(SOEM C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(ExternalProject)
ExternalProject_Add(soem_origin
  PREFIX soem
  GIT_REPOSITORY  https://github.com/OpenEtherCATsociety/SOEM.git
  GIT_TAG v1.4.0
  CMAKE_ARGS
   -DBUILD_TESTS=OFF
   -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/soem_install
  INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/soem_install"
)

install (
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/soem_install/
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

include_directories(${CMAKE_BINARY_DIR}/soem_install/include/soem)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  include_directories(${CMAKE_BINARY_DIR}/soem/src/soem_origin/oshw/win32/wpcap/Include)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${CMAKE_BINARY_DIR}/soem/src/soem_origin/oshw/win32/wpcap/Lib/x64)
  endif()
endif()

link_directories(${CMAKE_BINARY_DIR}/soem_install/lib)
add_executable(soem_test "server.c" "simple_test.c")

target_link_libraries(soem_test soem)
if(WIN32)
  target_link_libraries(soem_test ws2_32.lib Packet.lib wpcap.lib Winmm.lib)
else()
  target_link_libraries(soem_test pthread rt)
endif()
  
add_dependencies(soem_test soem_origin)
