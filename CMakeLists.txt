cmake_minimum_required(VERSION 3.22)
set(TARGET QUASAR-gs)
project(${TARGET})

set(QUASAR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QUASAR)
set(APP_COMMON_DIR ${QUASAR_DIR}/apps/Common)
set(APP_COMMON_SHADERS_DIR ${APP_COMMON_DIR}/shaders)

# QUASAR
set(QUASAR_BUILD_APPS OFF) # disable building apps
add_subdirectory(${QUASAR_DIR})

# QUASAR App Common
add_subdirectory(${APP_COMMON_DIR})

# App source files
file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create executable for each file in apps/
file(GLOB_RECURSE APP_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp")
foreach(APP_SRC ${APP_SRCS})
    get_filename_component(APP_NAME ${APP_SRC} NAME)
    string(REPLACE ".cpp" "" APP_NAME ${APP_NAME})
    add_executable(${APP_NAME} ${APP_SRC} ${SRCS})

    target_include_directories(${APP_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${APP_COMMON_DIR}/include
        ${APP_COMMON_SHADERS_DIR}/include
    )
    target_link_libraries(${APP_NAME} PRIVATE quasar quasar_common)
endforeach()

file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders_gs SYMBOLIC)

file(CREATE_LINK ${QUASAR_DIR}/renderer/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders_builtin SYMBOLIC)
file(CREATE_LINK ${QUASAR_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets SYMBOLIC)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/test.ply ${CMAKE_CURRENT_BINARY_DIR}/test.ply SYMBOLIC)
