cmake_minimum_required(VERSION 3.9)
project(SOEM
    DESCRIPTION "Simple Open EtherCAT Master"
    VERSION 1.4.0
    LANGUAGES C)

set(SOURCES
    osal
    oshw
    soem
    osal/win32
    oshw/win32
    oshw/win32/wpcap/Include
  )

include_directories(${SOURCES})

file(GLOB SOEM_SOURCES soem/*.c)
file(GLOB OSAL_SOURCES osal/win32/*.c)
file(GLOB OSHW_SOURCES oshw/win32/*.c)

set(SOURCE_CPP_FILES
  ${SOEM_SOURCES}
  ${OSAL_SOURCES}
  ${OSHW_SOURCES}
  main.c
)
link_directories(${CMAKE_CURRENT_LIST_DIR}/oshw/win32/wpcap/Lib/x64)
set(OS_LIBS wpcap.lib Packet.lib Ws2_32.lib Winmm.lib)

add_executable(test ${SOURCE_CPP_FILES})

target_link_libraries(test ${OS_LIBS})