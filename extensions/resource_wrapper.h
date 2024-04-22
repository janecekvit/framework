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
#include "extensions/extensions.h"

#include <functional>

namespace janecekvit::extensions
{
/// <summary>
/// The wrapper implemented own deleter that gains functionality to release all used resources correctly.
/// Method derives from the getter_setter class to using input resource with implicit conversions.
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
#if (__cplusplus >= __cpp_lib_concepts)
template <class _Resource, class _E = constraints::default_exception_callback>
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
		deleter_missing_exception(const std::type_info& typeInfo) noexcept
		{
			using namespace std::string_literals;
			_text = "resource_wrapper: Deleter with specified type cannot be found: "s + typeInfo.name();
		}
		const char* what() const override
		{
			return _text.data();
		}

	protected:
		std::string _text;
	};

public:
	using accessor		   = typename std::function<void(_Resource&)>;
	using const_accessor   = typename std::function<void(const _Resource&)>;
	using resource_deleter = typename accessor;

public:
	virtual ~resource_wrapper() noexcept
	{
		_resource.reset();
#if __cplusplus >= __cpp_lib_concepts

		try
		{
			_check_deleter();
		}
		catch (const std::exception& ex)
		{
			if constexpr (!std::is_same_v<_E, constraints::default_exception_callback>)
				_exceptionCallback(ex);
		}

#endif
	}

	constexpr resource_wrapper() = delete;

	constexpr resource_wrapper(resource_deleter&& deleter)
		: _resource(_create_resource(_Resource{}, resource_deleter(deleter)))
		, _deleter(std::forward<resource_deleter>(deleter))
	{
	}

	template <class _FwdResource>
	constexpr resource_wrapper(_FwdResource&& resource, resource_deleter&& deleter)
		: _resource(_create_resource(std::forward<_Resource>(resource), resource_deleter(deleter)))
		, _deleter(std::forward<resource_deleter>(deleter))
	{
	}

	constexpr resource_wrapper(const _Resource& resource, resource_deleter&& deleter)
		: _resource(_create_resource(resource, resource_deleter(deleter)))
		, _deleter(std::forward<resource_deleter>(deleter))
	{
	}

#if __cplusplus >= __cpp_lib_concepts
	constexpr resource_wrapper(const _Resource& resource, resource_deleter&& deleter, _E&& fnExceptionCallback)
		requires(std::is_invocable_v<_E, const std::exception&>)
		: _resource(_create_resource(resource, resource_deleter(deleter)))
		, _deleter(std::forward<resource_deleter>(deleter))
		, _exceptionCallback(std::forward<_E>(fnExceptionCallback))
	{
	}
#endif

	constexpr resource_wrapper(const resource_wrapper& other)
#if __cplusplus >= __cpp_lib_concepts
		requires std::is_copy_constructible_v<_Resource>
#endif
		: _resource(other._resource)
		, _deleter(other._deleter)
		, _exceptionCallback(other._exceptionCallback)
	{
	}

#if __cplusplus >= __cpp_lib_concepts
	constexpr resource_wrapper(const resource_wrapper&)
		requires !
				 std::is_copy_constructible_v<_Resource> = delete;
#endif

	constexpr resource_wrapper(resource_wrapper&& other) noexcept
		: _resource(std::move(other._resource))
		, _deleter(std::move(other._deleter))
	{
	}

	constexpr resource_wrapper& operator=(const resource_wrapper& other)
#if __cplusplus >= __cpp_lib_concepts
		requires std::is_copy_constructible_v<_Resource>
#endif
	{
		_deleter  = other._deleter;
		_resource = other._resource;
		return *this;
	}

#if __cplusplus >= __cpp_lib_concepts
	constexpr resource_wrapper& operator=(const resource_wrapper&)
		requires !
				 std::is_copy_constructible_v<_Resource> = delete;
#endif

	constexpr resource_wrapper& operator=(resource_wrapper&& other) noexcept
	{
		_deleter  = std::move(other._deleter);
		_resource = std::move(other._resource);
		return *this;
	}

	constexpr resource_wrapper& operator=(const _Resource& resource)
	{
		_check_deleter();
		_resource = _create_resource(resource, resource_deleter(_deleter));
		return *this;
	}

	constexpr resource_wrapper& operator=(_Resource&& resource)
	{
		_check_deleter();
		_resource = _create_resource(std::move(resource), resource_deleter(_deleter));
		return *this;
	}

	constexpr void reset()
	{
		_check_deleter();
		_resource = _create_resource(nullptr, resource_deleter(_deleter));
	}

	constexpr void retrieve(const_accessor&& fnAccess) const
	{
		fnAccess(*_resource);
	}

	constexpr void update(accessor&& fnAccess)
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

	template <class _Quantified = _Resource, std::enable_if_t<constraints::is_explicitly_convertible_v<_Quantified, bool>, int> = 0>
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
	[[nodiscard]] constexpr std::shared_ptr<_Resource> _create_resource(_Resource&& resource, resource_deleter&& deleter)
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

protected:
	void _check_deleter() const
	{
		if (!_deleter)
			throw deleter_missing_exception(typeid(std::declval<resource_deleter>()));
	}

protected:
	std::shared_ptr<_Resource> _resource{};
	resource_deleter _deleter{};
	const _E _exceptionCallback{};
};

} // namespace janecekvit::extensions
