cmake_minimum_required(VERSION 3.11)
project(test)

#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_C_FLAGS "-Wall -fPIC")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(LXPANEL lxpanel)
pkg_check_modules(LIBNOTIFY libnotify)

include_directories(${GTK2_INCLUDE_DIRS})
link_directories(${GTK2_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED test.c)
target_link_libraries(${PROJECT_NAME} ${GTK2_LIBRARIES} ${LIBNOTIFY_LIBRARIES})
