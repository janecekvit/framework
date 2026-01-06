#pragma once
/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2025 Vit janecek <mailto:janecekvit@outlook.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

compiler_support.h
Purpose:	header for compiler support

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
*/

#include <memory>

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
