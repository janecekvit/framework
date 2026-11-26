# Builds a minimal consumer of framework::framework, the way an external project uses it:
# through find_package on an installed package, or through add_subdirectory when
# FRAMEWORK_SOURCE_DIR points at a framework checkout.
cmake_minimum_required(VERSION 3.25)
project(framework_package_test LANGUAGES CXX)

set(FRAMEWORK_SOURCE_DIR "" CACHE PATH "Framework checkout to consume as a subproject instead of the installed package")

if(FRAMEWORK_SOURCE_DIR)
    add_subdirectory(${FRAMEWORK_SOURCE_DIR} framework)
    if(TARGET framework_tests)
        message(FATAL_ERROR "The framework subproject must not build its own tests")
    endif()
else()
    find_package(framework REQUIRED)
endif()

add_executable(framework_package_test main.cpp)
target_link_libraries(framework_package_test PRIVATE framework::framework)
