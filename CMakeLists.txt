cmake_minimum_required(VERSION 3.21)
project(VJStudio VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

qt_add_executable(VJStudio
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/clipwidget.h
    src/clipwidget.cpp
)

target_link_libraries(VJStudio PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

if(MSVC)
    target_compile_options(VJStudio PRIVATE /W4)
else()
    target_compile_options(VJStudio PRIVATE -Wall -Wextra -Wpedantic)
endif()
