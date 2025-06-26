#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

resource_wrapper.h
Purpose:	header file contains RAII pattern

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.06 10/06/2021
*/

#include "extensions/constraints.h"

#include <functional>

namespace janecekvit::storage
{
/// <summary>
/// The wrapper implemented own deleter that gains functionality to release all used resources correctly.
/// Ensure that deleter's body can be called multiple times to handle deallocations of the same resource in case of CopyConstructible methods of this wrapper are used.
/// When _DestuctorThrowException is set to true, destructor can raise exception when deleter functor fails or isn't initialized.
/// Throwing an exception out of a destructor is dangerous. If another exception is already propagating the application will terminate. Be careful with this.
/// </summary>
/// <example>
/// <code>
/// auto oWrapperFile = resource_wrapper<std::fstream>(std::fstream("Test", std::ios::binary), [](std::fstream& i)
/// {
///		i.close();
/// });
///	bool open = oWrapperFile->is_open();
/// </code>
/// </example>
#ifdef __cpp_lib_concepts
template <class _Resource, class _ResourceDeleter = std::function<void(_Resource&)>, class _ExceptionCallback = constraints::default_exception_callback>
#else
template <class _Resource>
#endif
class resource_wrapper
{
public:
	class deleter_missing_exception
		: public std::exception
	{
	public:
		deleter_missing_exception(const std::type_info& type_info) noexcept
		{
			using namespace std::string_literals;
			_text = "resource_wrapper: Deleter with specified type cannot be found: "s + type_info.name();
		}

		const char* what() const noexcept override
		{
			return _text.data();
		}

	private:
		std::string _text;
	};

public:
	using accessor = typename std::function<void(_Resource&)>;
	using const_accessor = typename std::function<void(const _Resource&)>;

public:
	virtual ~resource_wrapper() noexcept
	{
		_resource.reset();
#ifdef __cpp_lib_concepts

		try
		{
			_check_deleter();
		}
		catch (const std::exception& ex)
		{
			if constexpr (!std::is_same_v<_ExceptionCallback, constraints::default_exception_callback>)
				_exception_callback(ex);
		}

#endif
	}

	constexpr resource_wrapper() = delete;

	constexpr resource_wrapper(_ResourceDeleter&& deleter)
		: _resource(_create_resource(_Resource{}, _ResourceDeleter(deleter), _ExceptionCallback(_exception_callback)))
		, _deleter(std::forward<_ResourceDeleter>(deleter))
	{
	}

	constexpr resource_wrapper(_Resource&& resource, _ResourceDeleter&& deleter)
		: _resource(_create_resource(std::forward<_Resource>(resource), _ResourceDeleter(deleter), _ExceptionCallback(_exception_callback)))
		, _deleter(std::forward<_ResourceDeleter>(deleter))
	{
	}

	constexpr resource_wrapper(const _Resource& resource, _ResourceDeleter&& deleter)
		: _resource(_create_resource(resource, _ResourceDeleter(deleter), _ExceptionCallback(_exception_callback)))
		, _deleter(std::forward<_ResourceDeleter>(deleter))
	{
	}

#ifdef __cpp_lib_concepts
	constexpr resource_wrapper(_Resource&& resource, _ResourceDeleter&& deleter, _ExceptionCallback&& fnExceptionCallback)
		requires(std::is_invocable_v<_ExceptionCallback, const std::exception&>)
		: _resource(_create_resource(std::forward<_Resource>(resource), _ResourceDeleter(deleter), _ExceptionCallback(fnExceptionCallback)))
		, _deleter(std::forward<_ResourceDeleter>(deleter))
		, _exception_callback(std::forward<_ExceptionCallback>(fnExceptionCallback))
	{
	}
#endif

	constexpr resource_wrapper(const resource_wrapper& other)
#ifdef __cpp_lib_concepts
		requires(std::is_copy_constructible_v<_Resource>)
#endif
		: _resource(other._resource)
		, _deleter(other._deleter)
		, _exception_callback(other._exception_callback)
	{
	}

#ifdef __cpp_lib_concepts
	constexpr resource_wrapper(const resource_wrapper&)
		requires(!std::is_copy_constructible_v<_Resource>)
	= delete;
#endif

	constexpr resource_wrapper(resource_wrapper&& other) noexcept
		: _resource(std::move(other._resource))
		, _deleter(std::move(other._deleter))
	{
	}

	constexpr resource_wrapper& operator=(const resource_wrapper& other)
#ifdef __cpp_lib_concepts
		requires(std::is_copy_constructible_v<_Resource>)
#endif
	{
		_deleter = other._deleter;
		_resource = other._resource;
		return *this;
	}

#ifdef __cpp_lib_concepts
	constexpr resource_wrapper& operator=(const resource_wrapper&)
		requires(!std::is_copy_constructible_v<_Resource>)
	= delete;
#endif

	constexpr resource_wrapper& operator=(resource_wrapper&& other) noexcept
	{
		_deleter = std::move(other._deleter);
		_resource = std::move(other._resource);
		return *this;
	}

	constexpr resource_wrapper& operator=(const _Resource& resource)
	{
		_check_deleter();
#ifdef __cpp_lib_concepts
		_resource = _create_resource(resource, _ResourceDeleter(_deleter), _ExceptionCallback(_exception_callback));
#else
		_resource = _create_resource(resource, _ResourceDeleter(_deleter));
#endif
		return *this;
	}

	constexpr resource_wrapper& operator=(_Resource&& resource)
	{
		_check_deleter();
#ifdef __cpp_lib_concepts
		_resource = _create_resource(std::move(resource), _ResourceDeleter(_deleter), _ExceptionCallback(_exception_callback));
#else
		_resource = _create_resource(std::move(resource), _ResourceDeleter(_deleter));
#endif
		return *this;
	}

	constexpr void reset()
	{
		_check_deleter();
#ifdef __cpp_lib_concepts
		_resource = _create_resource(nullptr, _ResourceDeleter(_deleter), _ExceptionCallback(_exception_callback));
#else
		_resource = _create_resource(nullptr, _ResourceDeleter(_deleter));
#endif
	}

	constexpr void retrieve(const const_accessor& fnAccess) const
	{
		fnAccess(*_resource);
	}

	constexpr void update(const accessor& fnAccess)
	{
		fnAccess(*_resource);
	}

	[[nodiscard]] constexpr operator const _Resource&() const&
	{
		return *_resource;
	}

	[[nodiscard]] constexpr operator _Resource&() &
	{
		return *_resource;
	}

	[[nodiscard]] constexpr operator _Resource&&() &&
	{
		return std::move(*_resource);
	}

	template <class _Quantified = _Resource, std::enable_if_t<constraints::is_convertible_v<_Quantified, bool>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(*_resource);
	}

	template <class _Quantified = _Resource, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return _resource->begin();
	}

	template <class _Quantified = _Resource, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return _resource->end();
	}

	template <class _Quantified = _Resource, std::enable_if_t<constraints::is_container_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return _resource->size();
	}

	template <class _Quantified = _Resource, std::enable_if_t<std::is_pointer_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource&
	{
		return *_resource;
	}

	template <class _Quantified = _Resource, std::enable_if_t<std::is_pointer_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource&
	{
		return *_resource;
	}

	template <class _Quantified = _Resource, std::enable_if_t<!std::is_pointer_v<_Quantified> && !constraints::is_shared_ptr_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource*
	{
		return std::addressof(*_resource);
	}

	template <class _Quantified = _Resource, std::enable_if_t<!std::is_pointer_v<_Quantified> && !constraints::is_shared_ptr_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource*
	{
		return std::addressof(*_resource);
	}

	template <class _Quantified = _Resource, std::enable_if_t<!std::is_pointer_v<_Quantified> && constraints::is_shared_ptr_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) operator->() & noexcept
	{
		return *_resource;
	}

	template <class _Quantified = _Resource, std::enable_if_t<!std::is_pointer_v<_Quantified> && constraints::is_shared_ptr_v<_Quantified>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) operator->() const& noexcept
	{
		return *_resource;
	}

	[[nodiscard]] constexpr _Resource* operator&()
	{
		return std::addressof(*_resource);
	}

protected:
#ifdef __cpp_lib_concepts
	[[nodiscard]] constexpr std::shared_ptr<_Resource> _create_resource(_Resource&& resource, _ResourceDeleter&& deleter, _ExceptionCallback&& exceptionCallback)
	{
		return std::shared_ptr<_Resource>(new _Resource(std::forward<_Resource>(resource)), [stored_deleter = std::move(deleter), stored_callback = std::move(exceptionCallback)](_Resource* resource)
			{
				// call inner resource deleter
				try
				{
					if (stored_deleter)
						stored_deleter(*resource);
				}
				catch (const std::exception& ex)
				{
					if constexpr (!std::is_same_v<_ExceptionCallback, constraints::default_exception_callback>)
						stored_callback(ex);
				}

				delete resource;
				resource = nullptr;
			});
	}
#else
	[[nodiscard]] constexpr std::shared_ptr<_Resource> _create_resource(_Resource&& resource, _ResourceDeleter&& deleter)
	{
		return std::shared_ptr<_Resource>(new _Resource(std::forward<_Resource>(resource)), [stored_deleter = std::move(deleter)](_Resource* resource)
			{
				// call inner resource deleter
				if (stored_deleter)
					stored_deleter(*resource);

				delete resource;
				resource = nullptr;
			});
	}
#endif

protected:
	void _check_deleter() const
	{
		if (!_deleter)
			throw deleter_missing_exception(typeid(std::declval<_ResourceDeleter>()));
	}

protected:
	std::shared_ptr<_Resource> _resource{};
	_ResourceDeleter _deleter{};
	const _ExceptionCallback _exception_callback{};
};

#ifdef __cpp_lib_concepts
/// <summary>
/// User defined deduction guide CTAD for final_action using default exception callback
/// </summary>
template <class _Resource, std::invocable _ResourceDeleter>
resource_wrapper(_Resource&&, _ResourceDeleter&&)
	-> resource_wrapper<_Resource, _ResourceDeleter, constraints::default_exception_callback>;

/// <summary>
/// User-defined deduction guide CTAD for final_action using custom exception callback
/// </summary>
template <class _Resource, std::invocable _ResourceDeleter, std::invocable _ExceptionCallback>
resource_wrapper(const _Resource&, _ResourceDeleter&&, _ExceptionCallback&&)
	-> resource_wrapper<_Resource, std::function<void(_Resource&)>, _ExceptionCallback>;

#endif

} // namespace janecekvit::storage
