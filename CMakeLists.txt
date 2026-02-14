cmake_minimum_required(VERSION 3.16)

project(DLLChecker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Concurrent)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Set UTF-8 encoding for MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/peparser.cpp
    src/pathresolver.cpp
    src/dependencyscanner.cpp
    src/comparisonengine.cpp
    src/reportgenerator.cpp
    src/dllcollector.cpp
    src/scanworker.cpp
    src/inputvalidator.cpp
    src/logger.cpp
    resources/resources.qrc
)

# Header files
set(HEADERS
    include/mainwindow.h
    include/peparser.h
    include/pathresolver.h
    include/dependencyscanner.h
    include/comparisonengine.h
    include/reportgenerator.h
    include/dllcollector.h
    include/scanworker.h
    include/logger.h
    include/inputvalidator.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Link Qt libraries
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Concurrent
)

# Link Windows libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        imagehlp
        version
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/Release
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
