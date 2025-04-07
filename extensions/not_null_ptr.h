#pragma once

#include "exception/exception.h"
#include "extensions/constraints.h"

#include <concepts>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace janecekvit::extensions
{

/// <summary>
/// A wrapper that ensures a pointer is never null
/// </summary>
/// <typeparam name="T">The pointer type to wrap (raw pointer or smart pointer)</typeparam>
template <constraints::pointer_type _Type>
class [[nodiscard]] not_null_ptr
{
	using deleter_type = std::conditional_t<std::is_pointer_v<_Type>, std::function<void()>, std::monostate>;

public:
	not_null_ptr() = delete;
	not_null_ptr(std::nullptr_t) = delete;
	not_null_ptr& operator=(std::nullptr_t) = delete;

	template <typename _Fwd>
		requires(constraints::is_shared_ptr_v<std::decay_t<_Fwd>> || constraints::is_unique_ptr_v<std::decay_t<_Fwd>>) && std::convertible_to<_Fwd, _Type>
	constexpr explicit not_null_ptr(_Fwd&& ptr)
		: _pointer(std::forward<_Fwd>(ptr))
	{
		if (!_pointer)
			throw exception::exception("Null pointer assignment to required_ptr");
	}

	template <typename _Fwd, typename _Deleter = std::default_delete<std::remove_pointer_t<_Type>>>
		requires std::is_pointer_v<std::decay_t<_Fwd>> && std::convertible_to<_Fwd, _Type>
	constexpr explicit not_null_ptr(_Fwd&& ptr, _Deleter deleter = _Deleter())
		: _pointer(std::forward<_Fwd>(ptr))
	{
		if (!_pointer)
			throw exception::exception("Null pointer assignment to required_ptr");

		_deleter = [ptr, d = std::move(deleter)]()
		{
			if (ptr)
				d(ptr);
		};
	}

	template <typename _Fwd>
		requires std::is_pointer_v<std::decay_t<_Fwd>> && std::convertible_to<_Fwd, _Type>
	constexpr explicit not_null_ptr(_Fwd&& ptr, std::nullptr_t)
		: _pointer(std::forward<_Fwd>(ptr))
	{
		if (!_pointer)
			throw exception::exception("Null pointer assignment to required_ptr");
	}

	~not_null_ptr() noexcept
	{
		if constexpr (std::is_pointer_v<_Type>)
		{
			if (_deleter)
				_deleter();
		}
	}

	constexpr not_null_ptr(const not_null_ptr& other) = default;
	constexpr not_null_ptr(not_null_ptr&& other) noexcept = default;

	constexpr not_null_ptr& operator=(const not_null_ptr& other) = default;
	constexpr not_null_ptr& operator=(not_null_ptr&& other) noexcept = default;

	not_null_ptr& operator++() = delete;
	not_null_ptr& operator--() = delete;
	not_null_ptr operator++(int) = delete;
	not_null_ptr operator--(int) = delete;
	not_null_ptr& operator+=(std::ptrdiff_t) = delete;
	not_null_ptr& operator-=(std::ptrdiff_t) = delete;
	void operator[](std::ptrdiff_t) const = delete;

public:
	friend void swap(not_null_ptr<_Type>& lhs, not_null_ptr<_Type>& rhs) noexcept
	{
		std::swap(lhs._pointer, rhs._pointer);
	}

	[[nodiscard]] constexpr _Type& get() noexcept
	{
		return _pointer;
	}

	[[nodiscard]] constexpr const _Type& get() const noexcept
	{
		return _pointer;
	}

	constexpr auto operator->() const noexcept(noexcept(std::declval<_Type>().operator->()))
	{
		if constexpr (std::is_pointer_v<_Type>)
			return _pointer;
		else
			return _pointer.operator->();
	}

	[[nodiscard]] constexpr decltype(auto) operator*() const noexcept(noexcept(*std::declval<_Type>()))
	{
		return *_pointer;
	}

	constexpr operator _Type() const noexcept(noexcept(std::declval<_Type>()))
	{
		return _pointer;
	}

	constexpr bool operator==(std::nullptr_t) const noexcept
	{
		return false;
	}

	constexpr auto operator<=>(const not_null_ptr&) const = default;

private:
	_Type _pointer;
	deleter_type _deleter;
};

// Deduction guide
template <typename _Type>
not_null_ptr(_Type) -> not_null_ptr<_Type>;

template <constraints::pointer_type _Type, typename _Fwd>
inline constexpr not_null_ptr<_Type> make_required(_Fwd&& ptr)
{
	return not_null_ptr<_Type>(std::forward<_Fwd>(ptr));
}

template <typename _PointerType, typename... _Args>
	requires(
				std::same_as<_PointerType, std::unique_ptr<typename std::pointer_traits<_PointerType>::element_type>> ||
				std::same_as<_PointerType, std::shared_ptr<typename std::pointer_traits<_PointerType>::element_type>>) &&
			std::constructible_from<typename std::pointer_traits<_PointerType>::element_type, _Args...>
inline constexpr not_null_ptr<_PointerType> make_not_null_ptr(_Args&&... args)
{
	using _Pointee = typename std::pointer_traits<_PointerType>::element_type;
	if constexpr (std::is_same_v<_PointerType, std::unique_ptr<_Pointee>>)
	{
		return not_null_ptr(std::make_unique<_Pointee>(std::forward<_Args>(args)...));
	}
	else if constexpr (std::is_same_v<_PointerType, std::shared_ptr<_Pointee>>)
	{
		return not_null_ptr(std::make_shared<_Pointee>(std::forward<_Args>(args)...));
	}
	else
	{
		static_assert(constraints::always_false_v<_PointerType>,
			"Unsupported smart pointer type for in-place construction");
	}
}

template <typename _Pointee, typename... _Args>
	requires(std::constructible_from<_Pointee, _Args...>)
inline constexpr not_null_ptr<std::add_pointer_t<_Pointee>> make_not_null_ptr(_Args&&... args)
{
	return not_null_ptr<std::add_pointer_t<_Pointee>>(new _Pointee(std::forward<_Args>(args)...));
}

} // namespace janecekvit::extensions
