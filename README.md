# C++ Utility Library

This project provides a set of modern C++ utilities, including custom exception handling, thread-safe concurrent data structures, a heterogeneous container for storing and managing objects of various types, and a resource wrapper implementing the RAII pattern.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
  - [Exception Handling](#exception-handling)
  - [Synchronization and Concurrency](#synchronization-and-concurrency)
  - [Heterogeneous Container](#heterogeneous-container)
  - [Resource Wrapper](#resource-wrapper)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
  - [Exception Handling](#exception-handling-1)
    - [Throwing Exceptions](#throwing-exceptions)
    - [Custom Exception Types](#custom-exception-types)
  - [Synchronization and Concurrency](#synchronization-and-concurrency-1)
    - [Exclusive Access Example](#exclusive-access-example)
    - [Concurrent Access Example](#concurrent-access-example)
  - [Heterogeneous Container](#heterogeneous-container-1)
    - [Creating and Inserting Items](#creating-and-inserting-items)
    - [Accessing Stored Values](#accessing-stored-values)
    - [Calling Stored Functions](#calling-stored-functions)
  - [Resource Wrapper](#resource-wrapper-1)
    - [Creating a Resource Wrapper](#creating-a-resource-wrapper)
    - [Accessing and Modifying the Resource](#accessing-and-modifying-the-resource)
    - [Exception Handling in Destructors](#exception-handling-in-destructors)
- [License](#license)
- [Author](#author)
- [Acknowledgments](#acknowledgments)

## Introduction

The library offers:

- **Exception Handling**: A custom exception mechanism that extends `std::exception`, providing detailed error messages with source `std::location_location`.
- **Extensions**: 
- **Synchronization and Concurrency**: A collection of thread-safe concurrent containers and synchronization primitives for efficient and safe multi-threaded programming.
- **Heterogeneous Container**: Enables storing and working with objects of different types in a single container, with type-safe access.
- **Resource Wrapper**: Implements the RAII (Resource Acquisition Is Initialization) pattern for managing resources with custom deleters, ensuring proper resource cleanup.

## Features

### Exception Handling

- **Custom Exception Class**: `janecekvit::exception::exception` extends `std::exception` for enhanced error reporting.
- **Formatted Error Messages**: Supports formatting error messages using `std::format` and `std::vformat`, with variadic templates.
- **Source Location Information**: Automatically includes file name, line number, column number, and function name in error messages using `std::source_location`.

### Synchronization and Concurrency

- **Thread-Safe Containers**: Provides wrappers around standard STL containers to make them thread-safe for concurrent use.
- **Resource Ownership**: The `resource_owner` class template owns a resource and provides thread-safe access through exclusive or concurrent holders.
- **Exclusive Access**: `exclusive_resource_holder` grants exclusive (write) access to the resource.
- **Concurrent Access**: `concurrent_resource_holder` allows multiple threads to read the resource concurrently.

### Heterogeneous Container

- **Storing Various Types**: `janecekvit::storage::heterogeneous_container` allows storing objects of different types in a single container.
- **Type-Safe Access**: Safely retrieve stored objects by their type.
- **Support for Known Types**: Supports basic types (e.g., `int`, `float`, `std::string`) and allows extending with user-defined types.
- **Iteration and Invocation**: Enables iterating over stored objects and calling functions stored in the container.

### Resource Wrapper

- **RAII Pattern Implementation**: The `janecekvit::storage::resource_wrapper` class template wraps resources and ensures they are properly released using a custom deleter when the wrapper goes out of scope.
- **Custom Deleters**: Allows specifying custom deleter functions to manage resources that require special cleanup procedures.
- **Exception Handling in Destructors**: Optionally allows handling exceptions in destructors via a custom exception callback, while being cautious of the risks involved.

## Requirements

- C++20 compliant compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+)
- Standard library support for concepts, formatting, source locations, and synchronization primitives

## Installation

Include the necessary directories in your project's include path:
git clone https://github.com/yourusername/yourrepository.git


Ensure that your project's build settings enable C++20 features.

## Usage

### Exception Handling

Include the exception header:


#### Throwing Exceptions

**Basic Exception Throwing**:
```cpp
void exampleFunction() { using namespace janecekvit::exception; throw exception("An error occurred."); }
```

**Exception with Formatted Message**:
```cpp
void exampleFunction() { using namespace janecekvit::exception; throw exception("Error {}: {}", stdmake_tuple(404, "Not Found")); }
```

**Exception with Source Location**:
```cpp
void anotherFunction() { using namespace janecekvit::exception; throw exception(stdsource_location::current(), "An error occurred in the function."); }
```


#### Custom Exception Types

You can create custom exceptions by inheriting from `janecekvit::exception::exception`:
```cpp
class MyException : public janecekvitexceptionexception { public: using exception::exception; };
```

**Usage**:
```cpp
void customExceptionFunction() { using namespace janecekvitexception; throw MyException("Custom error: {}", stdmake_tuple("Something went wrong")); }
```


### Synchronization and Concurrency

Include the concurrent header:
```cpp
#include "synchronization/concurrent.h"
```

#### Exclusive Access Example

Creating a thread-safe resource:
```cpp
using namespace janecekvitsynchronizationconcurrent;
resource_owner<std::unordered_map<int, int>> resourceMap;
// Exclusive access to modify the resource { auto exclusiveAccess = resourceMap.exclusive(); exclusiveAccess->emplace(1, 42); } // The resource is unlocked here
```

#### Concurrent Access Example

```cpp
// Concurrent access to read the resource { auto concurrentAccess = resourceMap.concurrent(); int value = concurrentAccess->at(1); } // Multiple threads can perform concurrent reads
```


### Heterogeneous Container

Include the heterogeneous container header:
```cpp
#include "storage/heterogeneous_container.h"
```

#### Creating and Inserting Items

```cpp
using namespace janecekvit::storage;
heterogeneous_container<> container;
container.emplace(42);                         // Stores an int container.emplace(stdstring("Hello World")); // Stores a stdstring container.emplace(3.14);                       // Stores a double
```


### Resource Wrapper

Include the resource wrapper header:


#### Creating a Resource Wrapper

**Wrapping a resource with a custom deleter**:


#### Accessing and Modifying the Resource

**Accessing the resource**:


#### Exception Handling in Destructors

By default, the `resource_wrapper` will catch exceptions thrown by the deleter function in its destructor. You can provide a custom exception callback to handle exceptions as needed.

**Using a custom exception callback**:


**Caution**: Throwing exceptions from destructors is dangerous, especially if another exception is already propagating. It can lead to program termination. Ensure that the exception callback handles exceptions appropriately.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

- **Vít Janeček** - [Your Profile](https://github.com/janecekvit)

## Acknowledgments

- Inspired by modern C++ best practices and concurrency patterns
- Utilizes C++20 features for enhanced type safety and expressiveness