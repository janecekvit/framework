#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

getter_setter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.09 01/06/2020
*/

#include "extensions/constraints.h"

namespace janecekvit::extensions
{
/// <summary>
/// Default setter that says that getter_setter wrapper is public.
/// </summary>
class default_setter
{
};

/// <summary>
/// Default getter that says that getter_setter wrapper is public.
/// </summary>
class default_getter
{
};

/// <summary>
/// The Getter/Setter wrapper implements C++ like assessors to modify inner value of the defined resource type (_Resource).
/// Wrapper via accessors can modify the visibility of set and get methods.
/// Instead of default setter or default getter assign just type of the class that holds the resource.
/// If you assign to the default setter the resource owner type, only resource owner can work with set methods.
/// If you assign to the default getter the resource owner type, only resource owner can work with set methods.
/// </summary>
/// <example>
/// <code>
/// class TestGetterSetter
/// {
/// public:
/// 	void AllAccessible()
/// 	{
/// 		Int = 5;
/// 		IntSetterPrivate = 6;//   -> private set
/// 		IntBothPrivate = 6;//  -> private set
///
/// 		Vec.begin();
/// 		VecSetterPrivate.begin();
/// 		VecBothPrivate.begin(); // -> private get
///
/// 		Vec->emplace_back(5);
/// 		VecSetterPrivate->emplace_back(5); // -> private set
/// 		VecBothPrivate->emplace_back(5);  //-> private set
/// 	}
///
/// public:
/// 	extensions::getter_setter<int> Int;
/// 	extensions::getter_setter<int, TestGetterSetter> IntSetterPrivate;
/// 	extensions::getter_setter<int, TestGetterSetter, TestGetterSetter> IntBothPrivate;
///
/// 	extensions::getter_setter<std::vector<int>> Vec;
/// 	extensions::getter_setter<std::vector<int>, TestGetterSetter> VecSetterPrivate;
/// 	extensions::getter_setter<std::vector<int>, TestGetterSetter, TestGetterSetter> VecBothPrivate;
/// };
///
/// void AnyStuff()
/// {
/// 	TestGetterSetter visibility;
///
/// 	visibility.Int = 5;
/// 	//visibility.IntSetterPrivate = 6;//   -> private set
/// 	//visibility.IntBothPrivate = 6;//  -> private set
///
/// 	visibility.Vec.begin();
/// 	visibility.VecSetterPrivate.begin();
/// 	// visibility.VecBothPrivate.begin(); -> private get
///
/// 	visibility.Vec->emplace_back(5);
/// 	//visibility.VecSetterPrivate->emplace_back(5);  -> private set
/// 	//visibility.VecBothPrivate->emplace_back(5);  -> private set
/// }
/// </code>
/// </example>
template <class _Resource, class _TSetter = default_setter, class _TGetter = default_getter>
class getter_setter
{
	friend _TGetter;
	friend _TSetter;

public:
	constexpr getter_setter() = default;
	virtual ~getter_setter()  = default;

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr getter_setter(_Resource&& resource)
		: _resource(std::move(resource))
	{
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr getter_setter(_Resource&& resource)
		: _resource(std::move(resource))
	{
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr getter_setter(const _Resource& resource)
		: _resource(resource)
	{
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr getter_setter(const _Resource& resource)
		: _resource(resource)
	{
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter(const getter_setter& other)
		: _resource(other._resource)
	{
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter(const getter_setter& other)
		: _resource(other._resource)
	{
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter(getter_setter&& other)
		: _resource(std::move(other._resource))
	{
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter(getter_setter&& other)
		: _resource(std::move(other._resource))
	{
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter& operator=(const getter_setter& other)
	{
		_resource = other._resource;
		return *this;
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter& operator=(const getter_setter& other)
	{
		_resource = other._resource;
		return *this;
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter& operator=(getter_setter&& other)
	{
		_resource = std::move(other._resource);
		return *this;
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	getter_setter& operator=(getter_setter&& other)
	{
		_resource = std::move(other._resource);
		return *this;
	}

public: // public get
	template <class _TModifier = _TGetter, std::enable_if_t<std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return _resource;
	}

private: // private get
	template <class _TModifier = _TGetter, std::enable_if_t<!std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return _resource;
	}

public: // public get
	template <class _TModifier = _TGetter, std::enable_if_t<std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() & noexcept
	{
		return _resource;
	}

private: // private get
	template <class _TModifier = _TGetter, std::enable_if_t<!std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() & noexcept
	{
		return _resource;
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return _resource;
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return _resource;
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(_resource);
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(_resource);
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(_resource);
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(_resource);
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto() + we can handle non-cost variable because bool is captured by value
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() noexcept
	{
		return static_cast<bool>(_resource);
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto() + we can handle non-cost variable because bool is captured by value
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() noexcept
	{
		return static_cast<bool>(_resource);
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return _resource.begin();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return _resource.begin();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return _resource.end();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return _resource.end();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return _resource.size();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return _resource.size();
	}

public: // public set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<std::is_pointer_v<_Quantified> && std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource&
	{
		return _resource;
	}

private: // private set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<std::is_pointer_v<_Quantified> && !std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource&
	{
		return _resource;
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_pointer_v<_Quantified> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource&
	{
		return _resource;
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_pointer_v<_Quantified> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource&
	{
		return _resource;
	}

public: // public set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<!std::is_pointer_v<_Quantified> && std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource*
	{
		return std::addressof(_resource);
	}

private: // private set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<!std::is_pointer_v<_Quantified> && !std::is_same_v<_TModifier, default_setter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> _Resource*
	{
		return std::addressof(_resource);
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<!std::is_pointer_v<_Quantified> && std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource*
	{
		return std::addressof(_resource);
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<!std::is_pointer_v<_Quantified> && !std::is_same_v<_TModifier, default_getter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const _Resource*
	{
		return std::addressof(_resource);
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr _Resource* operator&()
	{
		return std::addressof(_resource);
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, default_setter>, int> = 0>
	constexpr _Resource* operator&()
	{
		return std::addressof(_resource);
	}

protected:
	_Resource _resource = {};
};

} // namespace janecekvit::extensions