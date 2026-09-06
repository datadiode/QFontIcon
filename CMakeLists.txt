cmake_minimum_required(VERSION 3.14)

project(QFontIcon VERSION 1.0.0 LANGUAGES CXX)

option(QFONTICON_BUILD_EXAMPLE "Build the nice example" OFF)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
set(QT Qt${QT_VERSION_MAJOR})

find_package(${QT} COMPONENTS Core Gui Widgets REQUIRED)

add_library(QFontIcon SHARED
  src/qfonticon.cpp
  include/qfonticon.h
)

target_link_libraries(QFontIcon PUBLIC ${QT}::Core ${QT}::Gui ${QT}::Widgets)
target_include_directories(QFontIcon PUBLIC include)

if (WIN32)
  target_compile_definitions(QFontIcon PRIVATE QFONTICON_EXPORT=__declspec\(dllexport\))
  target_compile_definitions(QFontIcon INTERFACE QFONTICON_EXPORT=__declspec\(dllimport\))
else()
  target_compile_definitions(QFontIcon PUBLIC QFONTICON_EXPORT=)
endif()

if(QFONTICON_BUILD_EXAMPLE)
  add_executable(example example/main.cpp include/awesome.h example/fonts.qrc)
  set_target_properties(example PROPERTIES
    AUTOMOC ON
    AUTORCC ON
  )
  target_link_libraries(example PUBLIC QFontIcon)
endif()
