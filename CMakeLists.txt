cmake_minimum_required(VERSION 3.14)

project(file-browser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick QuickControls2 Concurrent Sql StateMachine)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

#install(TARGETS file-browser
#    BUNDLE DESTINATION .
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

option(DB_TEST "use test database" ON)

if (DB_TEST)
    add_compile_definitions(-DDB_TEST)
endif(DB_TEST)

option(STATIC_BUILD "make static build" OFF)

if (STATIC_BUILD)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MT>")
	message("using msvc static runtime")
endif(STATIC_BUILD)

add_subdirectory(core)
add_subdirectory(tests)
add_subdirectory(gui)
