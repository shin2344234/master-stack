cmake_minimum_required(VERSION 3.20)
project(MasterStack LANGUAGES CXX C)

# Master Stack: an ASI plugin for Crimson Desert that raises the stack size of
# every stackable item. It edits no game file. The game's item table is read
# once at startup and the plugin multiplies each record's own limit as it is
# read, which is why a game patch does not break it the way a data mod breaks.
#
# The feature also ships inside Private Storage Master. When both are installed
# this one applies and that one stands down, and both export the same interface
# from include/stack_api.h so Master Looter's Stacks tab can edit either.

if(NOT WIN32)
    message(FATAL_ERROR "Master Stack targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # clangd reads build/compile_commands.json
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static CRT, no redist needed

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# MinHook is here only for its HDE64 length decoder, which farhook uses to
# measure the bytes it steals. No MinHook hook is ever created.
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

add_library(MasterStack SHARED
    src/dllmain.cpp
    src/core/mod.cpp
    src/core/log.cpp
    src/core/paths.cpp
    src/core/settings.cpp
    src/game/mem.cpp
    src/game/farhook.cpp
    src/game/addresses.cpp
    src/stacks/stacks.cpp
    src/api/api.cpp
)

target_include_directories(MasterStack PRIVATE src include ${minhook_SOURCE_DIR}/src/hde)
target_link_libraries(MasterStack PRIVATE minhook)
target_compile_definitions(MasterStack PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(MasterStack PRIVATE /utf-8 /W3 /EHsc)

set_target_properties(MasterStack PROPERTIES PREFIX "" SUFFIX ".asi" OUTPUT_NAME "MasterStack")

if(MSVC)
    target_compile_options(MasterStack PRIVATE /guard:cf)
    target_link_options(MasterStack PRIVATE /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /guard:cf)
endif()

add_custom_command(TARGET MasterStack POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MasterStack>" "${CMAKE_SOURCE_DIR}/dist/"
    COMMENT "Staging dist/"
    VERBATIM
)
