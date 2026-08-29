cmake_minimum_required(VERSION 3.16)
project(sfta CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSSH2 REQUIRED libssh2)

add_executable(sfta
    src/main.cpp
    src/NetworkLayer.cpp
    src/Session.cpp
)

target_include_directories(sfta PRIVATE ${LIBSSH2_INCLUDE_DIRS})
target_link_libraries(sfta PRIVATE ${LIBSSH2_LIBRARIES})
target_compile_options(sfta PRIVATE ${LIBSSH2_CFLAGS_OTHER})

add_executable(sfta_tests
    src/SessionTest.cpp
    src/Session.cpp
    src/NetworkLayer.cpp
)

target_include_directories(sfta_tests PRIVATE ${LIBSSH2_INCLUDE_DIRS})
target_link_libraries(sfta_tests PRIVATE ${LIBSSH2_LIBRARIES})
