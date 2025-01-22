cmake_minimum_required(VERSION 3.25.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(AutoCCache)

project(KProject)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(minhook CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(TARGET "kpatch")
set(TARGET_SRC  src/dllmain.cpp)
set(TARGET_OUTPUT_NAME "kpatch")
add_library(${TARGET} SHARED ${TARGET_SRC})
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TARGET_OUTPUT_NAME})
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${TARGET} PRIVATE spdlog::spdlog_header_only)
# target_link_libraries(${TARGET} PUBLIC minhook::minhook)


# ------------ post build ------------
# copy compile_commands.json for clangd
if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION "${CMAKE_SOURCE_DIR}")
endif()
