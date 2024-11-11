#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

property.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.09 01/06/2020
*/

#include "extensions/constraints.h"

namespace janecekvit::extensions
{
/// <summary>
/// Default setter that says that property wrapper is public.
/// </summary>
class public_access
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
/// 	extensions::property<int> Int;
/// 	extensions::property<int, TestGetterSetter> IntSetterPrivate;
/// 	extensions::property<int, TestGetterSetter, TestGetterSetter> IntBothPrivate;
///
/// 	extensions::property<std::vector<int>> Vec;
/// 	extensions::property<std::vector<int>, TestGetterSetter> VecSetterPrivate;
/// 	extensions::property<std::vector<int>, TestGetterSetter, TestGetterSetter> VecBothPrivate;
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
template <class _Resource, class _TSetter = public_access, class _TGetter = public_access>
class property
{
	friend _TGetter;
	friend _TSetter;

public:
	constexpr property() = default;
	virtual ~property()	 = default;

	// Constructors
public: // public set
	template <class _FwdResource = _Resource, class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr property(_FwdResource&& resource)
		: _resource(std::forward<_FwdResource>(resource))
	{
	}

private: // private set
	template <class _FwdResource = _Resource, class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr property(_FwdResource&& resource)
		: _resource(std::forward<_FwdResource>(resource))
	{
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	property(property&& other)
		: _resource(std::forward<_Resource>(other._resource))
	{
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	property(property&& other)
		: _resource(std::forward<_Resource>(other._resource))
	{
	}

	// assign operators
public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(property&& other)
	{
		_resource = std::forward<_Resource>(other._resource);
		return *this;
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(property&& other)
	{
		_resource = std::forward<_Resource>(other._resource);
		return *this;
	}

	//user-defined conversions
public: // public get
	template <class _TModifier = _TGetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return _resource;
	}

private: // private get
	template <class _TModifier = _TGetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return _resource;
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return _resource;
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return _resource;
	}

public: // public set
	template <class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(_resource);
	}

private: // private set
	template <class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(_resource);
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(_resource);
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(_resource);
	}

	// container accessors
public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return _resource.begin();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return _resource.begin();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return _resource.end();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return _resource.end();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return _resource.size();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return _resource.size();
	}

	// pointer accessors
private:
	// Helper for arrow operator
	template <bool IsPointer>
	[[nodiscard]] constexpr auto arrow_impl() const noexcept -> std::conditional_t<IsPointer, const _Resource&, const _Resource*>
	{
		if constexpr (IsPointer)
		{
			return _resource;
		}
		else
		{
			return std::addressof(_resource);
		}
	}

	template <bool IsPointer>
	[[nodiscard]] constexpr auto arrow_impl() noexcept -> std::conditional_t<IsPointer, _Resource&, _Resource*>
	{
		if constexpr (IsPointer)
		{
			return _resource;
		}
		else
		{
			return std::addressof(_resource);
		}
	}

public: // public set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept
	{
		return arrow_impl<std::is_pointer_v<_Quantified>>();
	}

private: // private set
	template <class _Quantified = _Resource, class _TModifier = _TSetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept
	{
		return arrow_impl<std::is_pointer_v<_Quantified>>();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept
	{
		return arrow_impl<std::is_pointer_v<_Quantified>>();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _TGetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept
	{
		return arrow_impl<std::is_pointer_v<_Quantified>>();
	}

	// Address accessors
public: // public set
	template <class _TModifier = _TGetter, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr const _Resource* operator&() const& noexcept 
	{
		return std::addressof(_resource);
	}

private: // private set
	template <class _TModifier = _TGetter, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr const _Resource* operator&() const& noexcept 
	{
		return std::addressof(_resource);
	}

protected:
	_Resource _resource = {};
};

} // namespace janecekvit::extensions