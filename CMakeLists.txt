cmake_minimum_required(VERSION 3.16)
project(boombox)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt automoc, autouic, and autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt6 components
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Find libmpv using pkg-config (optional)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MPV QUIET mpv)
else()
    message(WARNING "PkgConfig not found, checking for libmpv headers directly")
endif()

# If pkg-config didn't find libmpv, try to find headers directly
if(NOT MPV_FOUND)
    # Check if mpv headers exist in the local include directory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/mpv/client.h")
        set(MPV_FOUND TRUE)
        set(MPV_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
        message(STATUS "Found mpv headers in local include directory")
    else()
        message(WARNING "libmpv not found. Audio playback will not be available.")
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MPV_FOUND)
    include_directories(${MPV_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/filemanager.cpp
    src/logger.cpp
    src/statemanager.cpp
    src/widgets/trackinfo.cpp
    src/widgets/seekbar.cpp
    src/widgets/controls.cpp
)

# Add audioengine only if MPV is found
if(MPV_FOUND)
    list(APPEND SOURCES src/audioengine.cpp)
endif()

# Header files (needed for MOC)
set(HEADERS
    include/mainwindow.h
    include/widgets/trackinfo.h
    include/widgets/seekbar.h
    include/widgets/controls.h
    include/filemanager.h
    include/logger.h
    include/statemanager.h
)

# Add audioengine header only if MPV is found
if(MPV_FOUND)
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
if(MPV_FOUND)
    target_link_libraries(boombox ${MPV_LIBRARIES})
    target_link_directories(boombox PRIVATE ${MPV_LIBRARY_DIRS})
    target_compile_definitions(boombox PRIVATE ${MPV_CFLAGS_OTHER})
else()
    message(WARNING "libmpv not found. Audio playback will not be available.")
endif()

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
