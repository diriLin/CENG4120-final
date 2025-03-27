cmake_minimum_required(VERSION 3.17)

message(STATUS "CUHK CENG4120 Final Project")
message(STATUS "project directory ${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}")
# set(CMAKE_EXE_LINKER_FLAGS "-static")

project(FPGA_ROUTE)
find_package(Boost COMPONENTS serialization REQUIRED)

set(PATH_SRC ${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES ${PATH_SRC}/*.cpp)
add_executable(route ${SRC_FILES})

target_include_directories(route PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(route Boost::serialization)
install(TARGETS route DESTINATION ${PROJECT_SOURCE_DIR})



# set(PATH_SRC_EVAL ${PROJECT_SOURCE_DIR}/evaluator)
# file(GLOB_RECURSE SRC_FILES_EVAL ${PATH_SRC_EVAL}/*.cpp)
# add_executable(eval ${SRC_FILES_EVAL})
# install(TARGETS eval DESTINATION ${PROJECT_SOURCE_DIR})


