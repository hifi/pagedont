cmake_minimum_required(VERSION 3.1.0)

project(PageDont)

set(QT_COMPONENTS Core Network Gui Widgets)
find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)

set(CMAKE_AUTOMOC ON)

set(pagedont_SOURCES
    src/BinaryStream.cpp
    src/BinaryStream.h
    src/main.cpp
)

add_executable(pagedont ${pagedont_SOURCES})
target_link_libraries(pagedont Qt5::Core Qt5::Network Qt5::Gui Qt5::Widgets)
