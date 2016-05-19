cmake_minimum_required(VERSION 3.1.3)

enable_testing()

project(estd_test_exe)

set(estd_test_source_files main.cpp test.cpp test_obj_storage.cpp test_poly_obj_storage.cpp test_memory_resource.cpp)

add_executable(estd_test ${estd_test_source_files})

target_include_directories(estd_test PUBLIC ../../googletest/googletest/include/)
target_include_directories(estd_test PUBLIC ../../googletest/googlemock/include/)
target_include_directories(estd_test PUBLIC ../../include/)
target_include_directories(estd_test PUBLIC ../include/)

set_property(TARGET estd_test PROPERTY CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_link_libraries(estd_test gmock)

add_test(test_estd estd_test)