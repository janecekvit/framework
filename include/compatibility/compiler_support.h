#pragma once
/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2025 Vit Janecek. All rights reserved.

compiler_support.h
Purpose:	header for compiler support

@author:  Vit Janecek
@mailto:  <mailto:janecekvit@outlook.com>
*/

#include <memory>
#include <version>

// Check for std::jthread support
#if defined(__cpp_lib_jthread) && __cpp_lib_jthread >= 201911L && defined(__has_include) && __has_include(<stop_token>)
#include <stop_token>
#define HAS_JTHREAD
#endif

// Check for std::format support with macOS runtime compatibility
// std::format requires to_chars for floating-point types, which is only available on macOS 13.3+
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L &&  (!defined(__APPLE__) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 130300)
#include <format>
#define HAS_STD_FORMAT
#endif

// Keeps a call out of line; GCC 14 IPA modref otherwise drops stores made before an exception that the caller catches
#if defined(_MSC_VER) && !defined(__clang__)
#define FRAMEWORK_NOINLINE __declspec(noinline)
#else
#define FRAMEWORK_NOINLINE [[gnu::noinline]]
#endif
