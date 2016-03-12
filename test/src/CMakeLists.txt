cmake_minimum_required(VERSION 2.8.1)

enable_testing()

project(estd_test_exe)

set(estd_test_source_files main.cpp test.cpp test_obj_storage.cpp test_poly_obj_storage.cpp)

add_executable(estd_test ${estd_test_source_files})

target_include_directories(estd_test PUBLIC ../../googletest/googletest/include/)
target_include_directories(estd_test PUBLIC ../../googletest/googlemock/include/)
target_include_directories(estd_test PUBLIC ../../include/)
target_include_directories(estd_test PUBLIC ../include/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

target_link_libraries(estd_test gmock)

add_test(test_estd estd_test)