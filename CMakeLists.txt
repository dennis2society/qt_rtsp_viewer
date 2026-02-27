cmake_minimum_required(VERSION 3.21)

project(QtRtspViewer
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Qt6 RTSP Viewer Application"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Multimedia
    MultimediaWidgets
    OpenGLWidgets
)

find_package(OpenCV REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/videoplayer.cpp
    src/videoplayer.h
    src/videoworker.cpp
    src/videoworker.h
    src/videoeffects.cpp
    src/videoeffects.h
    src/effectssidebar.cpp
    src/effectssidebar.h
    src/opencvqtprocessor.hpp
    src/opencvqtprocessor.cpp
)

# UI files (if you have them)
set(UI_FILES
    # src/mainwindow.ui
)

# Resource files (if you have them)
set(RESOURCE_FILES
    # resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::OpenGLWidgets
    ${OpenCV_LIBS}
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Automatically copy the 'opencv' folder to the binary output directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/opencv"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/opencv"
    COMMENT "Copying opencv folder to output directory"
)
