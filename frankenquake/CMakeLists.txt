cmake_minimum_required(VERSION 3.25)

project(frankenquake)

option(USE_SDL "use SDL2" ON)
set(GAME_NAME "quake")

if (USE_SDL)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIR})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(game/${GAME_NAME})
include_directories(qcommon)
include_directories(network)
include_directories(console)
include_directories(misc)
include_directories(client)

FILE(GLOB ref_soft_SOURCES render/ref_soft/*.c)
FILE(GLOB ref_gl_SOURCES render/ref_gl/*.c)
FILE(GLOB client_SOURCES client/*.c)
FILE(GLOB console_SOURCES console/*.c)
FILE(GLOB misc_SOURCES misc/*.c)
FILE(GLOB network_SOURCES network/*.c)
FILE(GLOB server_SOURCES server/*.c)
FILE(GLOB sound_SOURCES sound/*.c)
FILE(GLOB qcommon_SOURCES qcommon/*.c)
FILE(GLOB progs_SOURCES progs/*.c)
#FILE(GLOB ${GAME_NAME}_SOURCES game/${GAME_NAME}/*.c)
if (USE_SDL)
    FILE(GLOB sys_SOURCES sys/sdl/*.c)
else()
    FILE (GLOB sys_SOURCES sys/null/*.c)
endif()

add_library(ref_soft ${ref_soft_SOURCES})
add_library(ref_gl ${ref_gl_SOURCES})
target_compile_definitions(ref_gl PUBLIC GLQUAKE)
add_library(client ${client_SOURCES})
add_library(console ${console_SOURCES})
add_library(misc ${misc_SOURCES})
add_library(network ${network_SOURCES})
add_library(server ${server_SOURCES})
add_library(sound ${sound_SOURCES})
add_library(qcommon ${qcommon_SOURCES})
add_library(progs ${progs_SOURCES})
#add_library(${GAME_NAME} ${${GAME_NAME}_SOURCES})

add_executable(frankenquake ${sys_SOURCES}
${ref_soft_SOURCES}
#${ref_gl_SOURCES}
${client_SOURCES}
${console_SOURCES}
${misc_SOURCES}
${network_SOURCES}
${server_SOURCES}
${sound_SOURCES}
${qcommon_SOURCES}
${progs_SOURCES}
)
if (GLQUAKE)
# todo
else()
target_link_libraries(frankenquake
    # qcommon ref_soft client console  network server sound  progs misc m
    m
    # ${GAME_NAME}
)
endif()
if (USE_SDL)
    target_link_libraries(frankenquake SDL2)
endif()
