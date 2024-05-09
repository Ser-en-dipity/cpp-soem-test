cmake_minimum_required(VERSION 3.9)
project(SOEM
    DESCRIPTION "Simple Open EtherCAT Master"
    VERSION 1.4.0
    LANGUAGES C)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # Default to installing in SOEM source directory
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
endif()

if(WIN32)
  set(OS "win32")
  include_directories(oshw/win32/wpcap/Include)
  link_directories(${CMAKE_CURRENT_LIST_DIR}/oshw/win32/wpcap/Lib/x64)
  
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_NO_WARNINGS")
  set(OS_LIBS wpcap.lib Packet.lib Ws2_32.lib Winmm.lib)
else()
  set(OS "linux")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
  set(OS_LIBS pthread rt)
endif()

message(STATUS "OS is ${OS}")

file(GLOB SOEM_SOURCES soem/*.c)
file(GLOB OSAL_SOURCES osal/${OS}/*.c)
file(GLOB OSHW_SOURCES oshw/${OS}/*.c)

set(SOURCE_CPP_FILES
  ${SOEM_SOURCES}
  ${OSAL_SOURCES}
  ${OSHW_SOURCES}
)

add_library(soem STATIC
  ${SOURCE_CPP_FILES})

target_link_libraries(soem ${OS_LIBS})

target_include_directories(soem PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/soem>
  $<INSTALL_INTERFACE:include/soem>)

target_include_directories(soem PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/osal>
  $<INSTALL_INTERFACE:include/soem>)

target_include_directories(soem PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/osal/${OS}>
  $<INSTALL_INTERFACE:include/soem>)

target_include_directories(soem
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/oshw/${OS}>
  $<INSTALL_INTERFACE:include/soem>
  )

if(WIN32) 
  add_subdirectory(test/win32)
else()
  add_subdirectory(test/linux)
endif()