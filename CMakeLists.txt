cmake_minimum_required(VERSION 3.25)
project(Framework VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard based on compiler
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Using C++${CMAKE_CXX_STANDARD} standard")

# Options
option(BUILD_TESTING "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
option(ENABLE_CPPCHECK "Enable cppcheck" ON)
option(ENABLE_CLANG_FORMAT "Enable clang-format targets" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators, set per-configuration output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Debug information settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_options(-g -O0)
        set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "" FORCE)
        message(STATUS "Debug build (Linux): enabled debug symbols and disabled optimizations")
    elseif(WIN32 AND MSVC)
        add_compile_options(/Zi /Od)
        add_link_options(/DEBUG)
        message(STATUS "Debug build (Windows): enabled debug symbols and disabled optimizations")
    endif()
endif()

# Platform-specific settings (basic compiler setup only)
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_options(/utf-8 /permissive- /Zc:__cplusplus /MP)
elseif(UNIX)
    # Enable sanitizers globally if requested
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
        message(STATUS "Sanitizers enabled")
    endif()
    
    # Enable coverage for GCC
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND ENABLE_COVERAGE)
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Coverage enabled for GCC")
    endif()
endif()

# Define strict warnings for our code
function(add_strict_warnings TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        # MSVC warnings
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /WX                                   # High warning level + warnings as errors
            /we4189 /we4100 /we4101                   # Unused variables/parameters
            /we4701 /we4703                           # Uninitialized variables
            /we4456 /we4457 /we4458                   # Variable shadowing
        >
        
        # GCC warnings
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall -Wextra -Wpedantic -Werror         # Base warnings + errors
            -Wunused-lambda-capture                   # Unused lambda captures
            -Wunused-const-variable=2                 # Unused const variables (level 2)
            -Wuninitialized -Wmaybe-uninitialized    # Uninitialized variables
            -Wshadow                                  # Variable shadowing
            -Wno-missing-field-initializers          # Disable problematic warning
        >
        
        # Clang warnings
        $<$<CXX_COMPILER_ID:Clang>:
            -Wall -Wextra -Wpedantic -Werror         # Base warnings + errors
            -Wunused-lambda-capture                   # Unused lambda captures
            -Wunused-const-variable                   # Unused const variables
            -Wuninitialized                           # Uninitialized variables
            -Wshadow                                  # Variable shadowing
            -Wno-gnu-zero-variadic-macro-arguments   # Disable GNU extension warning
        >
    )
endfunction()

# Find packages
find_package(Threads REQUIRED)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "Vcpkg toolchain file")
endif()

# Framework library
set(FRAMEWORK_HEADERS
    exception/exception.h
    extensions/cloneable.h
    extensions/constraints.h
    extensions/extensions.h
    extensions/finally.h
    extensions/property.h
    extensions/lazy.h
    extensions/not_null_ptr.h
    storage/heterogeneous_container.h
    storage/parameter_pack.h
    storage/resource_wrapper.h
    synchronization/signal.h
    synchronization/atomic_concurrent.h
    synchronization/concurrent.h
    synchronization/lock_owner.h
    synchronization/wait_for_multiple_signals.h
    thread/async.h
    thread/sync_thread_pool.h
    tracing/trace.h
    utility/conversions.h
)

add_library(framework INTERFACE)
add_library(framework::framework ALIAS framework)

target_sources(framework INTERFACE
    FILE_SET HEADERS 
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${FRAMEWORK_HEADERS}
)

target_include_directories(framework 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(framework INTERFACE Threads::Threads)

# Windows specific libraries
if(WIN32)
    target_link_libraries(framework INTERFACE 
        bcrypt Iphlpapi Crypt32 Secur32 Cryptnet
    )
endif()

## Source files for static analysis
file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_SOURCE_DIR}/*.cpp
    ${CMAKE_SOURCE_DIR}/*.h
    ${CMAKE_SOURCE_DIR}/*.hpp
    ${CMAKE_SOURCE_DIR}/*.inl
)

# Exclude build directory and external dependencies
list(FILTER SOURCE_FILES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")
list(FILTER SOURCE_FILES EXCLUDE REGEX "build/.*")
list(FILTER SOURCE_FILES EXCLUDE REGEX "_deps/.*")

# Testing
if(BUILD_TESTING)
    enable_testing()
    
    include(FetchContent)
    
    # Variables MUST be set BEFORE FetchContent_Declare
    cmake_policy(SET CMP0077 NEW)
    
    # Disable everything gmock related
    set(BUILD_GMOCK OFF)
    set(gmock_build_tests OFF)  
    set(gtest_build_tests OFF)
    set(gtest_build_samples OFF)
    set(INSTALL_GTEST OFF)
    set(INSTALL_GMOCK OFF)
    set(BUILD_SHARED_LIBS OFF)
    set(gtest_force_shared_crt OFF)
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
    )
    
    FetchContent_MakeAvailable(googletest)
    
    # Manual check - fail if GMock exists
    if(TARGET gmock OR TARGET gmock_main)
        message(FATAL_ERROR "GMock was built despite BUILD_GMOCK=OFF!")
    endif()
    
    set(TEST_SOURCES
        tests/test_async.cpp
        tests/test_atomic_concurrent.cpp
        tests/test_cloneable.cpp
        tests/test_constraints.cpp
        tests/test_property.cpp
        tests/test_signal.cpp
        tests/test_concurrent.cpp
        tests/test_extensions.cpp
        tests/test_sync_thread_pool.cpp
        tests/test_exception.cpp
        tests/test_finally.cpp
        tests/test_heterogeneous_container.cpp
        tests/test_lazy.cpp
        tests/test_parameter_pack.cpp
        tests/test_resource_wrapper.cpp
        tests/test_trace.cpp
        tests/test_wait_for_multiple_signals.cpp
        tests/test_not_null_ptr.cpp
        tests/test_lock_owner.cpp
    )
    
    add_executable(framework_tests ${TEST_SOURCES})
    
    # Apply strict warnings only to our test code
    add_strict_warnings(framework_tests)
    
    target_link_libraries(framework_tests 
        PRIVATE 
            framework 
            gtest
            gtest_main
    )
    
    target_include_directories(framework_tests PRIVATE tests)
    
    # Debug symbols for tests
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_compile_options(framework_tests PRIVATE -g -O0)
        elseif(WIN32 AND MSVC)
            target_compile_options(framework_tests PRIVATE /Zi /Od)
            target_link_options(framework_tests PRIVATE /DEBUG)
        endif()
    endif()
    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(framework_tests)
    
    message(STATUS "✅ Testing enabled with strict warnings")
endif()

# CppCheck
if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK 
            "${CPPCHECK_EXE}"
            "--enable=all"
            "--inconclusive"
            "--force"
            "--inline-suppr"
            "--suppress=missingInclude"
        )
        message(STATUS "✅ CppCheck enabled")
    endif()
endif()

# ======================
#     CUSTOM TARGETS
# ======================

# Clear target
add_custom_target(CLEAN_BUILD
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMENT "Cleaning build artifacts..."
    VERBATIM
)

# Framework target
if(BUILD_TESTING)
    add_custom_target(BUILD_FRAMEWORK
        DEPENDS framework framework_tests
        COMMENT "Building framework and tests"
        VERBATIM
    )
    message(STATUS "✅ BUILD_FRAMEWORK target: builds framework + tests")
else()
    add_custom_target(BUILD_FRAMEWORK
        DEPENDS framework
        COMMENT "Building framework only"
        VERBATIM
    )
    message(STATUS "✅ BUILD_FRAMEWORK target: builds framework only")
endif()

# Static analysis
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        
        # Manual clang-tidy check target
        add_custom_target(STATIC_ANALYSIS_CHECK
            COMMAND ${CLANG_TIDY_EXE}
                --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
                --header-filter=.*
                ${SOURCE_FILES}
                --
                -std=c++${CMAKE_CXX_STANDARD}
                -I${CMAKE_SOURCE_DIR}
                $<$<PLATFORM_ID:Windows>:-D_WIN32_WINNT=0x0601>
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy analysis"
            VERBATIM
        )
        
        # Manual clang-tidy fix target
        add_custom_target(STATIC_ANALYSIS_FIX
            COMMAND ${CLANG_TIDY_EXE}
                --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
                --header-filter=.*
                --fix
                --fix-errors
                ${SOURCE_FILES}
                --
                -std=c++${CMAKE_CXX_STANDARD}
                -I${CMAKE_SOURCE_DIR}
                $<$<PLATFORM_ID:Windows>:-D_WIN32_WINNT=0x0601>
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy with automatic fixes (USE WITH CAUTION!)"
            VERBATIM
        )
        
        message(STATUS "✅ Static analysis targets enabled")
    else()
        message(WARNING "clang-tidy not found. Install it to use static analysis targets.")
    endif()
endif()

# Clang-Format integration
if(ENABLE_CLANG_FORMAT)
    find_program(CLANG_FORMAT_EXE NAMES "clang-format")
    if(CLANG_FORMAT_EXE)
        message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
        
        # Format target - applies formatting
        add_custom_target(FORMAT
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Formatting code with clang-format"
            VERBATIM
        )
        
        # Show diff in console
        add_custom_target(FORMAT_CHECK
            COMMAND ${CMAKE_COMMAND} 
                -D CLANG_FORMAT_EXE=${CLANG_FORMAT_EXE}
                -D SOURCE_DIR=${CMAKE_SOURCE_DIR}
                -D CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                -P ${CMAKE_SOURCE_DIR}/cmake/formatShowDiff.cmake
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Showing formatting differences and saving to ${CMAKE_BINARY_DIR}/formatting.diff"
            VERBATIM
        )
        
        message(STATUS "✅ Code formatting targets enabled")
    else()
        message(WARNING "clang-format not found. Install it to use formatting targets.")
    endif()
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS framework
    EXPORT frameworkTargets
    FILE_SET HEADERS
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/framework
)

# Export targets
install(EXPORT frameworkTargets
    FILE frameworkTargets.cmake
    NAMESPACE framework::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/framework
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/frameworkConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/frameworkConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/framework
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/frameworkConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/frameworkConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/frameworkConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/framework
)