cmake_minimum_required(VERSION 3.5)

project(calc_resistor LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(calc_resistor
	form_rescalc.h form_rescalc.cpp form_rescalc.ui
	main.cpp)

install(TARGETS calc_resistor
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries(calc_resistor Qt6::Widgets)
