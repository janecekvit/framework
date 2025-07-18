cmake_minimum_required(VERSION 3.20)
project(Framework VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to latest available
if(MSVC)
	# MSVC standard
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    
    # MSVC uses /std:c++latest
    add_compile_options(/std:c++latest)
    
    # Set experimental  Standard Library Modules
    add_compile_options(/experimental:module)

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    message(STATUS "MSVC detected - using C++23 with /std:c++latest")
else()
	# Try to use the latest C++ standard available
    include(CheckCXXCompilerFlag)
    
    # Check for C++26 support
	CHECK_CXX_COMPILER_FLAG("-std=c++26" COMPILER_SUPPORTS_CXX26)
	CHECK_CXX_COMPILER_FLAG("-std=c++2c" COMPILER_SUPPORTS_CXX2C)

	# Check for C++23 support
	CHECK_CXX_COMPILER_FLAG("-std=c++23" COMPILER_SUPPORTS_CXX23)
	CHECK_CXX_COMPILER_FLAG("-std=c++2b" COMPILER_SUPPORTS_CXX2B)
    
    # Set the highest available standard
	if(COMPILER_SUPPORTS_CXX26 OR COMPILER_SUPPORTS_CXX2C)
        set(CMAKE_CXX_STANDARD 26)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-std=c++26)
            message(STATUS "GCC detected - using C++26 with -std=c++26")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-std=c++2c)  # Clang používá -std=c++2c pro C++26
            message(STATUS "Clang detected - using C++26 with -std=c++2c")
        endif()
    elseif(COMPILER_SUPPORTS_CXX23 OR COMPILER_SUPPORTS_CXX2B)
        set(CMAKE_CXX_STANDARD 23)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-std=c++23)
            message(STATUS "GCC detected - using C++23 with -std=c++23")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-std=c++2b)  # Clang používá -std=c++2b pro C++23
            message(STATUS "Clang detected - using C++23 with -std=c++2b")
        endif()
    else()
        set(CMAKE_CXX_STANDARD 20)
        message(STATUS "Using C++20 standard as fallback")
    endif()
    
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

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

# Platform-specific settings
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    add_compile_options(/utf-8 /permissive- /Zc:__cplusplus)
    
    # Enable parallel build
    add_compile_options(/MP)
    
    # Set warning level
    # add_compile_options(/W4 /WX) //TODO /WX
	add_compile_options(/W4)
elseif(UNIX)
    # GCC/Clang options
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wno-missing-field-initializers)
      
        if(ENABLE_COVERAGE)
            add_compile_options(--coverage)
            add_link_options(--coverage)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)
        
        # Clang doesn't support -std=c++latest, will use CMAKE_CXX_STANDARD
    endif()
    
    # Enable sanitizers
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Find packages
find_package(Threads REQUIRED)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "Vcpkg toolchain file")
endif()


# Clear target that works from any directory
add_custom_target(CLEAN_BUILD
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMENT "Cleaning build artifacts..."
    VERBATIM
)

# Framework library
set(FRAMEWORK_HEADERS
    exception/exception.h
    extensions/cloneable.h
    extensions/constraints.h
    extensions/extensions.h
    extensions/finally.h
    extensions/property.h
    extensions/lazy.h
    storage/heterogeneous_container.h
    storage/parameter_pack.h
    storage/resource_wrapper.h
    synchronization/signal.h
    synchronization/atomic_concurrent.h
    synchronization/concurrent.h
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

## Source files
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

# Windows specific libraries
if(WIN32)
    target_link_libraries(framework INTERFACE 
        bcrypt 
        Iphlpapi 
        Crypt32 
        Secur32 
        Cryptnet
    )
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    
    include(FetchContent)
    
    # Variables MUST be set BEFORE FetchContent_Declare
    cmake_policy(SET CMP0077 NEW)
    
    # Nuclear option - disable everything gmock related
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
    )
    
    add_executable(framework_tests ${TEST_SOURCES})
    target_link_libraries(framework_tests 
        PRIVATE 
            framework 
             gtest
			 gtest_main
    )
    target_include_directories(framework_tests PRIVATE tests)
    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(framework_tests)
endif()

# Static analysis
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        
        # Define clang-tidy command with config
        set(CLANG_TIDY_COMMAND 
            "${CLANG_TIDY_EXE}"
            "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
            "--header-filter=.*"
        )
        
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
        
    else()
        message(WARNING "clang-tidy not found. Install it to use static analysis targets.")
    endif()
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
    endif()
endif()

# Clang-Format #integration
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