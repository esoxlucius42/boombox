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

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
message(STATUS "Using required GStreamer backend")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(GSTREAMER_INCLUDE_DIRS)
    include_directories(${GSTREAMER_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/audioengine.cpp
    src/gstaudioengine.cpp
    src/filemanager.cpp
    src/playbackcontroller.cpp
    src/logger.cpp
    src/statemanager.cpp
    src/widgets/trackinfo.cpp
    src/widgets/seekbar.cpp
    src/widgets/controls.cpp
    src/widgets/albumart.cpp
)

# Header files (needed for MOC)
set(HEADERS
    include/audioengine.h
    include/gstaudioengine.h
    include/mainwindow.h
    include/playbackcontroller.h
    include/widgets/trackinfo.h
    include/widgets/seekbar.h
    include/widgets/controls.h
    include/widgets/albumart.h
    include/filemanager.h
    include/logger.h
    include/statemanager.h
)

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

# Link GStreamer backend
target_link_libraries(boombox ${GSTREAMER_LIBRARIES})
if(GSTREAMER_LIBRARY_DIRS)
    target_link_directories(boombox PRIVATE ${GSTREAMER_LIBRARY_DIRS})
endif()
if(GSTREAMER_CFLAGS_OTHER)
    target_compile_options(boombox PRIVATE ${GSTREAMER_CFLAGS_OTHER})
endif()
if(GSTREAMER_LDFLAGS_OTHER)
    target_link_options(boombox PRIVATE ${GSTREAMER_LDFLAGS_OTHER})
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
