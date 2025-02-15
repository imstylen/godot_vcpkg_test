cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

add_executable(HelloWorld main.cpp)

find_package(PkgConfig)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
target_link_libraries(HelloWorld PRIVATE PkgConfig::POPPLER_CPP)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg_installed/x64-windows/include)
target_link_directories(HelloWorld PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg_installed/x64-windows/lib)

target_link_libraries(HelloWorld PRIVATE cairo)