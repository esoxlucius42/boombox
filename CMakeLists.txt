cmake_minimum_required(VERSION 3.16)
project(boombox)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Qt automoc and autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt6 components
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Find libmpv using pkg-config (optional)
find_package(PkgConfig QUIET)
set(MPV_LINKABLE FALSE)
set(MPV_LINK_MODE "stub")
set(MPV_HEADER_FOUND FALSE)
if(PkgConfig_FOUND)
    pkg_check_modules(MPV QUIET mpv)
    if(MPV_FOUND AND MPV_LIBRARIES)
        if(MPV_INCLUDE_DIRS)
            set(MPV_HEADER_FOUND TRUE)
        else()
            find_path(MPV_HEADER_DIR
                NAMES mpv/client.h
                PATHS /usr/include /usr/local/include
            )
            if(MPV_HEADER_DIR)
                set(MPV_INCLUDE_DIRS "${MPV_HEADER_DIR}")
                set(MPV_HEADER_FOUND TRUE)
            endif()
        endif()
    endif()

    if(MPV_FOUND AND MPV_LIBRARIES AND MPV_HEADER_FOUND)
        set(MPV_LINKABLE TRUE)
        set(MPV_LINK_MODE "pkg-config")
        message(STATUS "Found linkable libmpv via pkg-config")
    endif()
endif()

# Fallback: allow linking directly to libmpv runtime library when pkg-config metadata
# is unavailable (common on immutable systems with runtime-only package layering).
if(NOT MPV_LINKABLE)
    find_library(MPV_FALLBACK_LIBRARY
        NAMES mpv
        PATHS /usr/lib64 /usr/lib /lib64 /lib
    )

    if(NOT MPV_FALLBACK_LIBRARY)
        find_file(MPV_FALLBACK_LIBRARY
            NAMES libmpv.so.2 libmpv.so.1
            PATHS /usr/lib64 /usr/lib /lib64 /lib
        )
    endif()

    if(MPV_FALLBACK_LIBRARY)
        find_path(MPV_HEADER_DIR
            NAMES mpv/client.h
            PATHS /usr/include /usr/local/include
        )
    endif()

    if(MPV_FALLBACK_LIBRARY AND MPV_HEADER_DIR)
        set(MPV_LINKABLE TRUE)
        set(MPV_LIBRARIES "${MPV_FALLBACK_LIBRARY}")
        set(MPV_INCLUDE_DIRS "${MPV_HEADER_DIR}")
        set(MPV_LINK_MODE "direct-library")
        message(STATUS "Found linkable libmpv via direct library lookup: ${MPV_FALLBACK_LIBRARY}")
    elseif(MPV_FALLBACK_LIBRARY)
        message(WARNING "Found libmpv runtime library but no mpv/client.h header. Building with compat declarations.")
        set(MPV_LINKABLE TRUE)
        set(MPV_LIBRARIES "${MPV_FALLBACK_LIBRARY}")
        set(MPV_LINK_MODE "direct-library-compat-header")
    endif()
endif()

if(NOT MPV_LINKABLE)
    message(WARNING "libmpv not found or not linkable. Building with stub backend (no real audio playback).")
else()
    message(STATUS "Using libmpv backend detection mode: ${MPV_LINK_MODE}")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MPV_LINKABLE AND MPV_INCLUDE_DIRS)
    include_directories(${MPV_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/audioengine.cpp
    src/filemanager.cpp
    src/playbackcontroller.cpp
    src/logger.cpp
    src/statemanager.cpp
    src/widgets/trackinfo.cpp
    src/widgets/seekbar.cpp
    src/widgets/controls.cpp
    src/widgets/albumart.cpp
    src/widgets/spectrumanalyzer.cpp
)

# Use libmpv stub only when real libmpv is not available
if(NOT MPV_LINKABLE)
    list(APPEND SOURCES src/mpv_stub.cpp)
endif()

# Header files (needed for MOC)
set(HEADERS
    include/mainwindow.h
    include/playbackcontroller.h
    include/widgets/trackinfo.h
    include/widgets/seekbar.h
    include/widgets/controls.h
    include/widgets/albumart.h
    include/widgets/spectrumanalyzer.h
    include/filemanager.h
    include/logger.h
    include/statemanager.h
)

# Add audioengine header only if MPV is found
if(MPV_LINKABLE)
    list(APPEND HEADERS include/audioengine.h)
endif()

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(boombox ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(boombox
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Test executable for logger
add_executable(test_logger_exe
    src/main_test.cpp
    src/logger.cpp
)

target_link_libraries(test_logger_exe
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Link MPV if found
if(MPV_LINKABLE)
    target_link_libraries(boombox ${MPV_LIBRARIES})
    if(MPV_LIBRARY_DIRS)
        target_link_directories(boombox PRIVATE ${MPV_LIBRARY_DIRS})
    endif()
    if(MPV_CFLAGS_OTHER)
        target_compile_options(boombox PRIVATE ${MPV_CFLAGS_OTHER})
    endif()
    if(NOT MPV_INCLUDE_DIRS)
        target_compile_definitions(boombox PRIVATE BOOMBOX_USING_MPV_COMPAT_HEADER=1)
    endif()
else()
    target_compile_definitions(boombox PRIVATE BOOMBOX_USING_MPV_STUB=1)
endif()

# Test executable for error handling
add_executable(test_error_handling
    test_error_handling.cpp
    src/filemanager.cpp
    src/logger.cpp
)

target_link_libraries(test_error_handling
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Compiler flags for Release optimization
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(boombox PRIVATE /O2 /W4)
    else()
        target_compile_options(boombox PRIVATE -O2 -Wall -Wextra)
    endif()
else()
    if(NOT MSVC)
        target_compile_options(boombox PRIVATE -Wall -Wextra)
    endif()
endif()
