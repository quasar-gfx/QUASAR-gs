cmake_minimum_required(VERSION 3.22)
set(TARGET gs_viewer)
project(${TARGET})

set(CMAKE_CXX_STANDARD 17)

set(QUASAR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QUASAR)
set(APP_COMMON_DIR ${QUASAR_DIR}/apps/Common)
set(APP_COMMON_SHADERS_DIR ${APP_COMMON_DIR}/shaders)

# QUASAR
set(QUASAR_BUILD_APPS OFF) # disable building apps
add_subdirectory(${QUASAR_DIR})

# QUASAR App Common
add_subdirectory(${APP_COMMON_DIR})

# app source files
file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${TARGET} ${SRCS})
target_include_directories(${TARGET}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${APP_COMMON_DIR}/include
    ${APP_COMMON_SHADERS_DIR}/include
)
target_link_libraries(${TARGET} PRIVATE quasar quasar_common)

file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders_gs SYMBOLIC)

file(CREATE_LINK ${QUASAR_DIR}/renderer/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders_builtin SYMBOLIC)
file(CREATE_LINK ${QUASAR_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets SYMBOLIC)
file(CREATE_LINK ../test.ply ${CMAKE_CURRENT_BINARY_DIR}/test.ply SYMBOLIC)
