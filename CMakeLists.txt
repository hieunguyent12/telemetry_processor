cmake_minimum_required(VERSION 3.26)
project(telemetry_processor)

set(CMAKE_CXX_STANDARD 17)

file(GLOB files
        "*.h"
        "*.cpp"
)

add_subdirectory(cparse)
add_executable(telemetry_processor
        ${files}
)

target_link_libraries(telemetry_processor PUBLIC cparse)


find_package(sockpp CONFIG REQUIRED)
target_link_libraries(telemetry_processor PRIVATE Sockpp::sockpp-static)

find_package(libuv CONFIG REQUIRED)
target_link_libraries(telemetry_processor PRIVATE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)

target_include_directories(telemetry_processor PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/cparse")