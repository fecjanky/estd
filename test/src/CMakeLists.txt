cmake_minimum_required(VERSION 3.1.3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(config)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 14)

enable_testing()

project(estd_test_exe)

file(GLOB estd_test_source_files  *.cpp)
file(GLOB estd_test_header_files  ../include/*.h)
file(GLOB estd_header_files  ../../include/*.h)
set(HEADERS
        ${estd_test_header_files}
        ${estd_header_files}
)
add_executable(estd_test  ${estd_test_source_files} ${HEADERS})

target_include_directories(estd_test PUBLIC ../../googletest/googletest/include/)
target_include_directories(estd_test PUBLIC ../../googletest/googlemock/include/)
target_include_directories(estd_test PUBLIC ../../include/)
target_include_directories(estd_test PUBLIC ../include/)


target_link_libraries(estd_test gmock)
set_property(TARGET estd_test PROPERTY CXX_STANDARD 14)

add_test(NAME test_estd COMMAND estd_test WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
