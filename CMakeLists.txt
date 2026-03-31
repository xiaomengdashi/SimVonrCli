cmake_minimum_required(VERSION 3.20)

project(sim_vonr_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sim_vonr_cli
    src/main.cpp
)

target_include_directories(sim_vonr_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
