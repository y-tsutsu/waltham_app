cmake_minimum_required(VERSION 2.8)
project(waltham_study)

find_package(PkgConfig)
pkg_check_modules(WALTHAM REQUIRED waltham)

include_directories(${WALTHAM_INCLUDE_DIRS})
link_directories(${WALTHAM_LIBRARY_DIRS})

add_executable(server myserver.cpp)
target_link_libraries(server ${WALTHAM_LIBRARIES} -lev)

#add_executable(client client.cpp)
#target_link_libraries(client ${WALTHAM_LIBRARIES} -lboost_system -lpthread)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_BUILD_TYPE Debug)
