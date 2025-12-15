# C++ Utility Library

This project provides a set of modern C++ utilities, including custom exception handling, thread-safe concurrent data structures, a heterogeneous container for storing and managing objects of various types, and a resource wrapper implementing the RAII pattern.

## Table of Contents

- [Introduction](#introduction)
- [Requirements](#requirements)
- [Installation](#installation)
- [Features](#features)
  - [Exception Handling](#exception-handling)
  - [Extensions](#extensions)
	- [Cloneable](#cloneable)
	- [Extensions](#extensions-1)
	- [Finally](#finally)
	- [Lazy](#lazy)
	- [Not Null Pointer](#not-null-pointer)
	- [Property](#property)
  - [Storages](#storages)
	- [Heterogeneous Container](#heterogeneous-container)
	- [Parameter Pack](#parameter-pack)
	- [Resource Wrapper](#resource-wrapper)
  - [Synchronization primitives](#synchronization-primitives)
	- [Concurrent Data Structures](#concurrent-data-structures)
	- [Lock-owner Mechanisms](#lock-owner-mechanisms)
	- [Signalization](#signalization)
	- [Wait for Multiple Signals](#wait-for-multiple-signals)
  - [Multi-threaded Extensions](#multi-threaded-extensions)
	- [Async](#async)
	- [Sync Thread Pool](#sync-thread-pool)
  - [Tracing and Logging](#tracing-and-logging)
- [License](#license)
- [Author](#author)
- [Acknowledgments](#acknowledgments)

## Introduction

The library offers:

- **Exception Handling**: A custom exception mechanism that extends `std::exception`, providing detailed error messages with source `std::source_location`.
- **RAII Extensions**: Various extensions for better work with RAII patterns, deferred actions or properties.
- **Storages**: A collection of storage classes and utilities for managing resources, including a `heterogeneous_container`, `parameter_pack` and a `resource_wrapper`.
- **Synchronization primitives**: A collection of thread-safe concurrent containers and synchronization primitives for efficient and safe multi-threaded programming.
- **Multi-threaded extensions**: A collection of utilities for multi-threaded programming, including a `sync_thread_pool`.
- **Tracing and logging**: A collection of utilities for tracing and logging, including a `trace`.

## Requirements
- Minimal supported C++ standard: C++20
- Recommended C++ standard: C++23
- C++20 compliant compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+)
- Standard library support for concepts, formatting, source locations, and synchronization primitives

## Installation

Include the necessary directories in your project's include path:
git clone https://github.com/vitjanecek/framework.git

## Features

### Exception Handling

This header file, `exception/exception.h` and provides a custom exception handling mechanism in C++. 

It extends the standard std::exception to include detailed error messages with `std::source_location` and `std::thread::id` information.
The `throw_exception` template class simplifies throwing exceptions with formatted messages.

- **Custom exception class**: Extends std::exception for more informative error handling.
- **Formatted error messages**: Supports formatting error messages using std::format and std::vformat.
- **Unicode Support**: Handles both narrow and wide string formats for error messages.

```cpp
#include "exception/exception.h"
#include <iostream>

using namespace janecekvit;

try
{
	throw exception::exception("Error code {}: {}", std::make_tuple(404, "Not Found"));
	throw exception::exception(std::source_location::current(), std::this_thread::get_id(), "Error code {}: {}", 404, "Not Found");
}
catch (const std::exception& ex)
{
	std::cerr << ex.what() << std::endl;
}
```
\
Using `throw_exception` helper
```cpp
#include "exception/exception.h"
#include <iostream>

using namespace janecekvit;

try
{
	exception::throw_exception("Error code {}: {}", 404, "Not Found");
}
catch (const std::exception& ex)
{
	std::cerr << ex.what() << std::endl;
}
```

### Extensions

#### Cloneable
This header file, `extensions/cloneable.h` provides an interface for implementing the clone pattern in C++. 

It allows objects to be cloned, creating a deep copy wrapped in a smart pointer.

- **Clone Pattern**: Provides a standard interface for cloning objects.
- **Type Safety**: Ensures that the cloned object is of the same type as the original.
- **Smart Pointers**: Uses `std::unique_ptr` to manage the lifetime of the cloned objects, ensuring proper resource management.


```cpp
#include "extensions/cloneable.h"
#include <iostream>
#include <memory>

using namespace janecekvit;

struct test_class : public extensions::cloneable<test_class>
{
	int value;

	test_class(int val)
		: value(val)
	{
	}

	std::unique_ptr<test_class> clone() const override
	{
		return std::make_unique<test_class>(*this);
	}
};

test_class original(42);
std::unique_ptr<test_class> copy = original.clone();

std::cout << "Original value: " << original.value << std::endl;
std::cout << "Cloned value: " << copy->value << std::endl;
```

#### Extensions
This header file, `extensions/extensions.h` provides a set of extended methods implemented over STL containers. 
It includes utilities for container operations, type casting, tuple manipulation, numeric computations, and hash computations.

- **Container Operations**: Provides methods for executing operations on containers using keys and callbacks.
- **Type Casting**: Includes a method for casting std::unique_ptr to their inherited forms.
- **Tuple Manipulation**: Offers utilities for generating sequences, printing tuples, and streaming tuples.
- **Numeric Computations**: Includes a compile-time factorial computation.
- **Hash Computations**: Provides methods for combining hashes of multiple values.

```cpp
#include "extensions/extensions.h"
#include <unordered_map>
#include <iostream>

using namespace janecekvit;

// Container operation
std::unordered_map<int, int> mapInts;
mapInts.emplace(5, 10);
auto result = extensions::execute_on_container(mapInts, 5, [](int& value)
	{
		value += 1;
		return value;
	});
std::cout << "Container operation result: " << result << std::endl;

// Type casting
struct Base
{
	virtual ~Base() = default;
};

struct Derived : Base
{
	int value = 10;
};

std::unique_ptr<Base> basePtr = std::make_unique<Derived>();
std::unique_ptr<Derived> derivedPtr = extensions::recast<Base, Derived>(std::move(basePtr));

// Tuple manipulation
auto t = std::make_tuple(1, 2, 3);
std::stringstream ss = extensions::tuple::print(t, ", ");
std::cout << "Tuple: " << ss.str() << std::endl;

// Numeric computation
constexpr size_t factorialResult = extensions::numeric::factorial<5>::value;
std::cout << "Factorial of 5: " << factorialResult << std::endl;

// Hash computation
size_t hashValue = extensions::hash::combine(1, 2.5, std::string("test"));
std::cout << "Combined hash: " << hashValue << std::endl;
```

#### Finally
This header file, `extensions/finally.h` provide a mechanism to execute a callback function when an object goes out of scope.

It ensures that a specified callback function is executed when the scope ends, which is useful for resource cleanup.
This is similar to the "finally" block in other programming languages and is useful for ensuring that resources are released correctly.

- **Finally Semantics**: Ensures that a specified callback function is executed when the scope ends.
- **Exception Handling**: Optionally allows handling exceptions in the callback function via a custom exception callback.
- **Type Safety**: Uses C++ concepts and type traits to ensure that the callback function is invocable.

```cpp
#include "extensions/finally.h"
#include <fstream>
#include <iostream>

std::fstream file("test.txt", std::ios::out);
auto cleanup = janecekvit::extensions::finally([&]
	{
		file.close();
		std::cout << "Cleanup code executed." << std::endl;
	});

std::cout << "Function body." << std::endl;
// Cleanup code will be executed when `cleanup` goes out of scope
```
\
Using a custom exception callback:
```cpp
#include "extensions/finally.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

std::fstream file("test.txt", std::ios::out);
auto cleanup = janecekvit::extensions::finally([&]
	{
		if (file.is_open())
		{
			file.close();
			std::cout << "Cleanup code executed." << std::endl;
		}
		else
			throw std::runtime_error("Exception in cleanup.");
	},
	[](const std::exception& ex)
	{
		std::cerr << "Exception caught: " << ex.what() << std::endl;
	});

std::cout << "Function body." << std::endl;
// Cleanup code will be executed when `cleanup` goes out of scope
```

#### Lazy
This header file, `extensions/lazy.h` provides a lazy evaluation mechanism of stored callable functions in C++.

The `lazy_action` class template and the `lazy` function template provide a mechanism to store a callable function and its arguments, and to evaluate the function lazily when needed. 
- **Lazy Evaluation**: Stores a callable function and its arguments, and evaluates the function only when needed.
- **Flexible Invocation**: Allows invoking the stored function with the original arguments or with new arguments.

```cpp
#include "extensions/lazy.h"
#include <iostream>

using namespace janecekvit;

auto multiply = [](int a, int b)
{
	return a * b;
};
auto lazyMultiply = extensions::lazy(std::move(multiply), 2, 5);

// The function is not evaluated until this point
int result = lazyMultiply();
std::cout << "Result: " << result << std::endl; // Output: Result: 10

// The function can also be invoked with different arguments
int newResult = lazyMultiply(3, 6);
std::cout << "New Result: " << newResult << std::endl; // Output: New Result: 18
```

#### Not Null Pointer
This header file, `extensions/not_null_ptr.h` provides simple wrapper around a pointer that ensures the contained pointer is never nullptr. 

The `not_null_ptr` ensures a consistent pointer state throughout its lifetime. 
It includes optimizations to ensure minimal performance impact and supports all common pointer operations.

- **Guarantees Non-Null**: Ensures the pointer is never `nullptr`.
- **Works with Raw and Smart Pointers**: Supports raw pointers, `std::shared_ptr`, and `std::unique_ptr`.
- **In-Place Allocation**: Supports in-place memory allocation using `make_not_null_ptr`.
- **Raw pointer deallocation**: Supports raw pointer deallocation though custom deleter via constructor.

```cpp
#include "extensions/not_null_ptr.h"
#include <memory>
#include <iostream>

using namespace janecekvit;

auto intPtr = std::make_shared<int>(5);
extensions::not_null_ptr<std::shared_ptr<int>> ptr(intPtr);
std::cout << "Pointer value: " << *ptr << std::endl;

// In-place allocation
auto ptr2 = extensions::make_not_null_ptr<int>(42);
std::cout << "Pointer value: " << *ptr2 << std::endl;
```

#### Property
This header file, `extensions/property.h` aa way to define properties with customizable getter and setter functions in C++.

It allows for controlled access to the internal value of a resource, enabling encapsulation and data hiding. 
The visibility of the getter and setter methods can be modified using access control types.

- **Getter/Setter Mechanism**: Provides a way to define properties with getter and setter functions, can be assign directly or using lambda functions.
- **Access Control**: Allows modifying the visibility of getter and setter methods using access control types by set visibility to parent class `property<T, public_access, public_access>`.
- **Type Safety**: Ensures that the getter and setter functions are invocable with the provided arguments.
- **Flexible Initialization**: Supports various ways to initialize the property, including direct value assignment and lambda functions for custom getter and setter logic.

```cpp
#include "extensions/property.h"
#include <iostream>

using namespace janecekvit;

int hiddenValue = 10;
extensions::property<int> property(
    [&]() -> const int& { return hiddenValue; },
    [&](const int& newValue) { hiddenValue = newValue * newValue; }
);

std::cout << "Initial value: " << static_cast<int>(property) << std::endl;

property = 5;
std::cout << "Updated value: " << static_cast<int>(property) << std::endl;
```
\
Property visibility control in parent class
```cpp
#include "extensions/property.h"

using namespace janecekvit;

struct private_property
{ // private get and set
	extensions::property<int, private_property, private_property> value = 20;
};

struct public_property
{ // public get and set
	extensions::property<int> value = 20;
};

struct public_get_property
{ // public get and private set
	extensions::property<int, public_get_property> value = 20;
};

struct public_set_property
{ // private get and public set
	extensions::property<int, extensions::public_access, public_set_property> value = 20;
};

private_property privProperty;
// privProperty.value = 10;
// auto valuePriv = privProperty.value;

public_property pubProperty;
pubProperty.value = 10;
auto valuePub = pubProperty.value;

public_get_property pubGetProperty;
// pubGetProperty.value = 10;
const int& valuePubGet = const_cast<const public_get_property&>(pubGetProperty).value;

public_set_property pubSetProperty;
pubSetProperty.value = 10;
```

### Storages

#### Heterogeneous Container
This header file, `storage/heterogeneous_container.h` and provides a utility for storing and managing copy-constructible objects of various types in a single container. 

It implements the lazy evaluation idiom to enable processing input arguments as late as possible. 
The container supports type-safe access to the stored objects and provides various methods for retrieving and manipulating the stored data.

- **Lazy Evaluation**: Enables processing input arguments as late as possible.
- **Type-Safe Access**: Provides type-safe access to the stored objects.
- **Known types**: Known types are optimized for performance and they are stored in `std::variant`
- **Unknown types**: Unknown types rest of types and there are internally stored in std::any.
- **User defined types**: Extended known types e.g. `storage::heterogeneous_container<my_custom_type>` to improve performance impact.

```cpp
#include "storage/heterogeneous_container.h"
#include <iostream>
#include <string>
#include <functional>

using namespace janecekvit;

storage::heterogeneous_container<> container;

container.emplace(42);
container.emplace(std::string("Hello World"));
container.emplace(3.14);
container.emplace(std::function<int(int)>([](int x)
	{
		return x * 2;
	}));

int intValue = container.first<int>();
auto strings = container.get<std::string>();
for (const auto& str : strings)
{
	std::cout << str.get() << std::endl;
}

int result = container.call_first<std::function<int(int)>>(21);
std::cout << "Result: " << result << std::endl;
```

#### Parameter Pack
This header file, `storage/parameter_pack.h` and provides a utility for managing variadic arguments using the parameter pack pattern. 

It implements the lazy evaluation idiom to enable processing input arguments as late as possible.
It could be used in virtual interface functions or in functions that require a variable number of arguments.

- **Lazy Evaluation**: Enables processing input arguments as late as possible.
- **Variadic Argument Handling**: Supports forwarding and storing variadic arguments.
- **Type-Safe Retrieval**: Allows retrieving packed parameters by type.
- **Exception Handling**: Throws `std::invalid_argument` when the number of arguments received in `get` methods is incorrect.

```cpp
#include "storage/parameter_pack.h"
#include <iostream>
#include <string>

using namespace janecekvit;

// Create a parameter pack
storage::parameter_pack pack(42, std::string("Hello"), 3.14);

// Retrieve parameters as a tuple (C++20 and above)
auto [intValue, strValue, doubleValue] = pack.get_pack<int, std::string, double>();

// Print the retrieved values
std::cout << "Integer: " << intValue << std::endl;
std::cout << "String: " << strValue << std::endl;
std::cout << "Double: " << doubleValue << std::endl;
```

#### Resource Wrapper
This header file, `storage/resource_wrapper.h` provides a way to manage resources by the RAII pattern, ensuring that resources are properly released when the wrapper goes out of scope. 


- **RAII Pattern Implementation**: The template wraps resources and ensures they are properly released using a custom deleter when the wrapper goes out of scope.
- **Custom Deleters**: Allows specifying custom deleter functions to manage resources that require special cleanup procedures.
- **Exception Handling in Destructors**: Optionally allows handling exceptions in destructors via a custom exception callback, while being cautious of the risks involved.

```cpp
#include "storage/resource_wrapper.h"
#include <fstream>
#include <iostream>

using namespace janecekvit;

auto fileWrapper = storage::resource_wrapper(
	std::fstream("example.txt", std::ios::out),
	[](std::fstream& file)
	{
		if (file.is_open())
		{
			file.close();
		}
	});

if (fileWrapper->is_open())
	fileWrapper->clear();

fileWrapper.update([](std::fstream& file)
	{
		file << "Additional content." << std::endl;
	});
```
\
Using a custom exception callback:
```cpp
#include "storage/resource_wrapper.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace janecekvit;

auto fileWrapperWithExceptionHandling = storage::resource_wrapper(
		std::fstream("example.txt", std::ios::out),
		[](std::fstream& file)
		{
			// Custom deleter logic that might throw
			if (file.bad())
			{
				throw std::runtime_error("File stream is in a bad state.");
			}
			if (file.is_open())
			{
				file.close();
			}
		},
		[](const std::exception& ex)
		{
			// Custom exception handling logic
			std::cerr << "Exception in resource deleter: " << ex.what() << std::endl;
		});
```


### Synchronization primitives

#### Concurrent Data Structures
This header file, `synchronization/concurrent.h` provides a set of thread-safe concurrent containers and synchronization mechanisms that uses RAII pattern. 

It includes wrappers around standard STL containers to make them thread-safe and provides methods for exclusive and concurrent access to resources.

It provides the following key components:

- **`resource_owner`**: A template class that owns a resource and provides thread-safe access to it via exclusive or concurrent holders.
- **`exclusive_resource_holder`**: Grants exclusive (write) access to the resource, ensuring no other threads can access it concurrently.
- **`concurrent_resource_holder`**: Grants concurrent (read) access to the resource, allowing multiple threads to read the resource simultaneously.
- **Debugging Support**: In debug configuration or when `CONCURRENT_DBG_TOOLS` define is set, the library includes additional checks and lock detail tracking to help diagnose synchronization issues.

```cpp
#include "synchronization/concurrent.h"
#include <unordered_map>
#include <iostream>

using namespace janecekvit::synchronization;

concurrent::resource_owner<std::unordered_map<int, int>> resourceMap;
concurrent::unordered_map<int, int> resourceMapAlias;

// Exclusive access to modify the resource
{
	auto&& exclusiveAccess = resourceMap.exclusive();
	exclusiveAccess->emplace(1, 42);
} // The resource is unlocked here

// Concurrent(shared) access to read the resource
{
	auto&& concurrentAccess = resourceMap.concurrent();
	int value = concurrentAccess->at(1);
	std::cout << "Value: " << value << std::endl;
} // Multiple threads can perform concurrent reads
```
\
Example for getting information about the locks in debug mode
```cpp
#include "synchronization/concurrent.h"
#include <unordered_map>

using namespace janecekvit::synchronization;

// Release version - no lock tracking
concurrent::resource_owner_release<std::unordered_map<int, int>> resourceMapRelease;

// Debug version - compile-time enabled lock tracking
concurrent::resource_owner_debug<std::unordered_map<int, int>> resourceMapDebug;

// Runtime version - runtime-configurable lock tracking with custom callbacks
concurrent::resource_owner_runtime<std::unordered_map<int, int>> resourceMapRuntime;

// Build-dependent alias
// debug tracking when NDEBUG is not defined (debug build)
// no tracking when NDEBUG is defined (release build) or SYNCHRONIZATION_NO_TRACKING is defined
// runtime tracking when SYNCHRONIZATION_RUNTIME_TRACKING is defined
concurrent::unordered_map<int, int> resourceMapAlias;

// Get the lock details for tracked resource owners
auto exclusiveDetails = resourceMapDebug.get_exclusive_lock_details();
auto concurrentDetails = resourceMapDebug.get_concurrent_lock_details();
```
\
Example for runtime policy with custom callbacks
```cpp
#include "synchronization/concurrent.h"
#include <unordered_map>
#include <iostream>

using namespace janecekvit::synchronization;

// Enable tracking via callback
lock_tracking_runtime::set_callback([]()
{
    return true;
});

// Register static logging callback for lock events
lock_tracking_runtime::set_logging_callback(
    [](const lock_information& info, const void* mutex_ptr)
    {
        std::cout << "Lock acquired at: "
		  		  << info.Location.file_name() << ":"
		  		  << info.Location.line() << " for mutex: "
		  		  << mutex_ptr << std::endl;
    });

// Create runtime policy resource owner
concurrent::resource_owner_runtime<std::unordered_map<int, int>> resourceMap;

// Lock operations will now trigger the logging callback
{
    auto exclusive = resourceMap.exclusive();
    exclusive->emplace(1, 42);
}

// Disable tracking
lock_tracking_runtime::clear_callback();
lock_tracking_runtime::clear_logging_callback();
```

#### Lock-owner Mechanisms
This header file, `synchronization/lock_owner.h` provides a set of synchronizing primitives for managing thread synchronization.
It provides detailed information about the locks usage including the thread ID, Timestamp and `std::source_location`.

It provides the following key components:
- **`lock_owner`**: Provides a mechanism to manage the ownership of locks, including exclusive and concurrent locks.
- **`exclusive_lock_owner`**: A class that provides exclusive ownership of a lock, allowing only one thread to hold the lock at a time.
- **`concurrent_lock_owner`**: A class that provides concurrent ownership of a lock, allowing multiple threads to hold the lock simultaneously.
- **Debugging Support**: In debug configuration or when `CONCURRENT_DBG_TOOLS` define is set, the library includes additional checks and lock detail tracking to help diagnose synchronization issues.

```cpp
#include "synchronization/lock_owner.h"

using namespace janecekvit::synchronization;

synchronization::lock_owner<> lock;
bool nonThreadSafeLogic = false;

// Exclusive access
{
	auto&& exclusiveAccess = lock.exclusive();
	nonThreadSafeLogic = true; // Simulate some non-thread-safe logic
}

// Concurrent(shared) access
{
	auto&& concurrentAccess = lock.concurrent();
	auto result = nonThreadSafeLogic; // Simulate reading the resource
}
```
\
Example for getting information about the locks in debug mode
```cpp
#include "synchronization/lock_owner.h"

using namespace janecekvit::synchronization;

// Release version - no lock tracking
synchronization::lock_owner_release<> lockRelease;

// Debug version - compile-time enabled lock tracking
synchronization::lock_owner_debug<> lockDebug;

// Runtime version - runtime-configurable lock tracking with custom callbacks
synchronization::lock_owner_runtime<> lockRuntime;

// Build-dependent alias
// debug tracking when NDEBUG is not defined (debug build)
// no tracking when NDEBUG is defined (release build) or SYNCHRONIZATION_NO_TRACKING is defined
// runtime tracking when SYNCHRONIZATION_RUNTIME_TRACKING is defined
synchronization::lock_owner<> lockAlias;

// Get the lock details for tracked lock owners
auto exclusiveDetails = lockDebug.get_exclusive_lock_details();
auto concurrentDetails = lockDebug.get_concurrent_lock_details();
```
\
Example for runtime policy with custom callbacks
```cpp
#include "synchronization/lock_owner.h"
#include <iostream>

using namespace janecekvit::synchronization;

// Enable tracking via callback
lock_tracking_runtime::set_callback([]()
{
    return true;
});

// Register static logging callback for lock events
lock_tracking_runtime::set_logging_callback(
    [](const lock_information& info, const void* mutex_ptr)
    {
        std::cout << "Lock acquired at: "
		  		  << info.Location.file_name() << ":"
		  		  << info.Location.line() << " for mutex: "
		  		  << mutex_ptr << std::endl;
    });

// Create runtime policy lock owner
synchronization::lock_owner_runtime<> lock;

bool nonThreadSafeLogic = false;

// Lock operations will now trigger the logging callback
{
    auto exclusive = lock.exclusive();
    nonThreadSafeLogic = true;
}

// Disable tracking
lock_tracking_runtime::clear_callback();
lock_tracking_runtime::clear_logging_callback();
```


#### Signalization
This header file, `synchronization/signal.h` provides a mechanism for thread synchronization. 
It allows threads to wait for a signal and can be configured to reset automatically after each wait or to require manual reset. 
It supports different synchronization primitives such as `std::condition_variable_any`, `std::condition_variable`, and `std::binary_semaphore`.
Manual reset works only with `std::condition_variable_any`, `std::condition_variable`.


- **Thread Synchronization**: Provides a mechanism for synchronizing threads using signals.
- **Auto-Reset and Manual-Reset**: Supports both auto-reset and manual-reset modes (only `std::condition_variable`).
- **Flexible Synchronization Primitives**: Can be used with std::condition_variable_any, std::condition_variable, and std::binary_semaphore.
- **Predicate Support**: Allows waiting with a predicate to customize the wake-up condition.

Using `std::condition_variable_any` and `std::condition_variable`
```cpp
#include "synchronization/signal.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

using namespace janecekvit;

synchronization::signal<std::condition_variable, true> s; // manual reset = true

std::mutex mtx;
std::thread t([&]() mutable
	{
		std::unique_lock<std::mutex> lock(mtx);
		s.wait(lock);
		std::cout << "Worker thread signaled." << std::endl;
	});

std::this_thread::sleep_for(std::chrono::seconds(1));
s.signalize();

t.join();
```
\
Using `std::binary_semaphore`
```cpp
#include "synchronization/signal.h"
#include <semaphore>
#include <thread>
#include <chrono>
#include <iostream>

using namespace janecekvit;

synchronization::signal<std::binary_semaphore, true> s; // manual reset = true
std::thread t([&]() mutable
	{
		s.wait();
		std::cout << "Worker thread signaled." << std::endl;
	});

std::this_thread::sleep_for(std::chrono::seconds(1));
s.signalize();

t.join();
```

#### Wait for Multiple Signals
This header file, `synchronization/wait_for_multiple_signals.h` provides a mechanism for waiting on multiple signals in C++. 

It supports various synchronization primitives and allows threads to wait for multiple signals and retrieve the state of the signal that was triggered.
It supported synchronization primitives are, `std::condition_variable_any`, `std::condition_variable` and `std::binary_semaphore`.

- **Multiple Signal Waiting**: Allows threads to wait for multiple signals and retrieve the state of the signal that was triggered.
- **Flexible Synchronization Primitives**: Can be used with std::condition_variable_any, std::condition_variable, and std::binary_semaphore.
- **Predicate Support**: Allows waiting with a predicate to customize the wake-up condition.
- **Timeout Support**: Supports waiting with timeouts using wait_for and wait_until methods.

Using `std::condition_variable_any` and `std::condition_variable`
```cpp
#include "synchronization/wait_for_multiple_signals.h"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

using namespace janecekvit;

enum class signal_state
{
	signal1,
	signal2
};

synchronization::wait_for_multiple_signals<signal_state, std::condition_variable> wfms;

std::mutex mtx;
std::thread t([&]() mutable
	{
		std::unique_lock<std::mutex> lock(mtx);
		signal_state state = wfms.wait(lock);
		std::cout << "Worker thread signaled with state: " << static_cast<int>(state) << std::endl;
	});

std::this_thread::sleep_for(std::chrono::seconds(1));
wfms.signalize(signal_state::signal1);

t.join();
```
\
Using `std::binary_semaphore`
```cpp
#include "synchronization/wait_for_multiple_signals.h"
#include <semaphore>
#include <thread>
#include <chrono>
#include <iostream>

using namespace janecekvit;

enum class signal_state
{
	signal1,
	signal2
};

synchronization::wait_for_multiple_signals<signal_state, std::binary_semaphore> wfms;
std::thread t([&]() mutable
	{
		signal_state state = wfms.wait();
		std::cout << "Worker thread signaled with state: " << static_cast<int>(state) << std::endl;
	});

std::this_thread::sleep_for(std::chrono::seconds(1));
wfms.signalize(signal_state::signal1);

t.join();
```

### Multi-threaded Extensions

#### Async
This header file, `thread/async.h` provides a utility for creating asynchronous tasks using the C++ Standard Library's `std::async` function. 

It simplifies the creation of asynchronous tasks and returns a `std::future` object representing the result of the task.

```cpp
#include "thread/async.h"
#include <iostream>

using namespace janecekvit::thread;

auto future = async::create([](int a, int b)
	{
		return a + b;
	},
	3, 4);

int result = future.get(); // Wait for the task to complete and get the result
std::cout << "Task result: " << result << std::endl;
```


#### Sync Thread Pool
This header file, `thread/sync_thread_pool.h` provides a fixed-size thread pool that executes tasks from a queue. 

It supports adding tasks and waitable tasks, ensuring efficient task execution and synchronization.
The class uses various synchronization primitives to manage the task queue and worker threads.

- **Fixed-Size Thread Pool**: Manages a fixed number of worker threads to execute tasks.
- **Task Queue**: Supports adding tasks to a queue for execution by worker threads.
- **Waitable Tasks**: Allows adding tasks that return a future, enabling synchronization with task completion.
- **Move semantics**: Uses move semantics for tasks, ensuring efficient task management.

```cpp
#include "thread/sync_thread_pool.h"
#include <iostream>

using namespace janecekvit;

// Create a thread pool with 4 worker threads
thread::sync_thread_pool pool(4);

// Add a simple task to the thread pool
pool.add_task([]
	{
		std::cout << "Task executed." << std::endl;
	});

// Add a waitable task to the thread pool
auto future = pool.add_waitable_task([]
	{
		return 42;
	});

int result = future.get(); // Wait for the task to complete and get the result
std::cout << "Task result: " << result << std::endl;
```


#### Tracing and Logging

This header file, `tracing/trace.h`, provides a thread-safe utility for creating and managing trace events with support for blocking wait operations.

The `trace_event` class captures details about a trace event, including its priority, formatted message, `std::source_location` and `std::thread::id`.
The `trace` class manages a collection of trace events in a thread-safe queue, allowing for adding, retrieving, and flushing events with optional blocking wait functionality.

- **Formatted Trace Messages**: Supports formatted trace messages using `std::format`.
- **Source Location Information**: Captures `std::source_location` details such as file name, line number, and function name.
- **Thread Identification**: Captures the thread ID where the trace event was created.
- **Blocking Wait Operations**: Supports blocking wait for events with optional timeout using `std::condition_variable_any`.
- **Producer-Consumer Pattern**: Ideal for multi-threaded logging and event processing scenarios.

```cpp
#include "tracing/trace.h"
#include <string>
#include <iostream>

using namespace janecekvit;

enum class LogLevel
{
	Info,
	Warning,
	Error
};

tracing::trace<std::string, LogLevel> tracer;

tracer.create(tracing::trace_event{ LogLevel::Info, "This is a trace message with value: {}", 42 });

// Non-blocking call
auto event_opt = tracer.get_next_trace();
if (event_opt) {
	std::cout << "Trace event data: " << event_opt->data() << std::endl;
}

// Blocking call
auto event = tracer.get_next_trace_wait();
std::cout << "Trace event data: " << event.data() << std::endl;

// Blocking call with timeout
auto event_timeout = tracer.get_next_trace_wait_for(std::chrono::seconds(5));
if (event_timeout) {
	std::cout << "Trace event data: " << event_timeout->data() << std::endl;
} else {
	std::cout << "Timeout waiting for event" << std::endl;
}

tracer.flush();
```
## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

- **Vít Janeček** - [Your Profile](https://github.com/janecekvit)

## Acknowledgments

- Inspired by modern C++ best practices and concurrency patterns
- Utilizes C++20 features for enhanced type safety and expressiveness