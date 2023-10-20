cmake_minimum_required(VERSION 3.26)
project(telemetry_processor)

set(CMAKE_CXX_STANDARD 17)

add_executable(telemetry_processor main.cpp)

find_package(sockpp CONFIG REQUIRED)
target_link_libraries(telemetry_processor PRIVATE Sockpp::sockpp-static)
