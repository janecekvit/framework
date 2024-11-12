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
/// The property wrapper implements C++ like assessors to modify inner value of the defined resource type (_Resource).
/// Wrapper via accessors can modify the visibility of set and get methods.
/// Instead of default setter or default getter assign just type of the class that holds the resource.
/// If you assign to the default setter the resource owner type, only resource owner can work with set methods.
/// If you assign to the default getter the resource owner type, only resource owner can work with get methods.
/// </summary>
/// <example>
/// <code>
//   int hiddenValue = 10;
//   extensions::property<int> property([&]() -> const int&
//	 {
//	 	return hiddenValue;
//	 },
//	 [&](const int& newValue)
//	 {
//		hiddenValue = newValue * newValue;
//	 });
//
//   extensions::property<int> propertyNoLambda(10);
//
//   struct PropertyHolder
//   {
//		extensions::property<int, PropertyHolder, PropertyHolder> Value;
//   };

/// </code>
/// </example>
template <class _Resource, class _SetterAccess = public_access, class _GetterAccess = public_access>
class property
{
	friend _GetterAccess;
	friend _SetterAccess;

public:
	using getter = std::function<const _Resource&()>;
	using setter = std::function<void(const _Resource&)>;
	using const_resource = const _Resource&;

public:
	constexpr property() = default;
	virtual ~property() = default;

	// Constructors
public: // public set
	template <class _FwdResource = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr property(_FwdResource&& resource)
		: _resource(std::forward<_FwdResource>(resource))
	{
	}

private: // private set
	template <class _FwdResource = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr property(_FwdResource&& resource)
		: _resource(std::forward<_FwdResource>(resource))
	{
	}

public: // public set
	template <
		class _GetterFwd = getter,
		class _SetterFwd = setter,
		class _TModifier = _SetterAccess,
		std::enable_if_t<std::is_same_v<_TModifier, public_access> && std::is_same_v<std::invoke_result_t<std::decay_t<_GetterFwd>>, const_resource> && std::is_invocable_r_v<void, _SetterFwd, const _Resource&>, int> = 0>
	constexpr property(_GetterFwd&& get, _SetterFwd&& set)
		: _getter(std::forward<_GetterFwd>(get))
		, _setter(std::forward<_SetterFwd>(set))
	{
	}

private: // private set
	template <
		class _GetterFwd = getter,
		class _SetterFwd = setter,
		class _TModifier = _SetterAccess,
		std::enable_if_t<!std::is_same_v<_TModifier, public_access> && std::is_same_v<std::invoke_result_t<std::decay_t<_GetterFwd>>, const_resource> && std::is_invocable_r_v<void, _SetterFwd, const _Resource&>, int> = 0>
	constexpr property(_GetterFwd&& get, _SetterFwd&& set)
		: _getter(std::forward<getter>(get))
		, _setter(std::forward<setter>(set))
	{
	}

public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	property(property&& other)
		: _resource(std::forward<_Resource>(other._resource))
		, _getter(std::forward<getter>(other._getter))
		, _setter(std::forward<setter>(other._setter))
	{
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	property(property&& other)
		: _resource(std::forward<_Resource>(other._resource))
		, _getter(std::forward<getter>(other._getter))
		, _setter(std::forward<setter>(other._setter))
	{
	}

	// assign operators
public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(const property& other)
	{
		_resource = other._resource;
		_getter = other._getter;
		_setter = other._setter;
		return *this;
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(const property& other)
	{
		_resource = other._resource;
		_getter = other._getter;
		_setter = other._setter;
		return *this;
	}

public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(property&& other)
	{
		_resource = std::move(other._resource);
		_getter = std::move(other._getter);
		_setter = std::move(other._setter);
		return *this;
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	property& operator=(property&& other)
	{
		_resource = std::move(other._resource);
		_getter = std::move(other._getter);
		_setter = std::move(other._setter);
		return *this;
	}

public:
	template <class _FwdResource = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access> && std::is_convertible_v<std::decay_t<_FwdResource>, _Resource>, int> = 0>
	property& operator=(_FwdResource&& value)
	{
		set_detail(std::forward<_FwdResource>(value));
		return *this;
	}

private:
	template <class _FwdResource = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access> && std::is_convertible_v<std::decay_t<_FwdResource>, _Resource>, int> = 0>
	property& operator=(_FwdResource&& value)
	{
		set_detail(std::forward<_FwdResource>(value));
		return *this;
	}

	// set/get methods
private:
	template <class _FwdResource = _Resource>
	constexpr void set_detail(_FwdResource&& resource)
	{
		if (_setter)
			_setter(std::forward<_FwdResource>(resource));
		else
			_resource = std::forward<_FwdResource>(resource);
	}

	constexpr const _Resource& get_detail() const
	{
		if (_getter)
			return _getter();

		return _resource;
	}

public: // public set
	template <class _Input, class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access> && (std::is_same_v<_Input, _Resource> || std::is_same_v<_Input, setter>), int> = 0>
	constexpr void set(_Input&& value)
	{
		set_detail(std::forward<_Input>(value));
	}

private: // private set
	template <class _Input, class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	constexpr void set(_Input&& value)
	{
		set_detail(std::forward<_Input>(value));
	}

	// user-defined conversions
public: // public get
	template <class _TModifier = _GetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return get_detail();
	}

private: // private get
	template <class _TModifier = _GetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator const _Resource&() const& noexcept
	{
		return get_detail();
	}

public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return const_cast<_Resource&>(get_detail());
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&() & noexcept
	{
		return const_cast<_Resource&>(get_detail());
	}

public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(const_cast<_Resource&&>(get_detail()));
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr operator _Resource&&() && noexcept
	{
		return std::move(const_cast<_Resource&&>(get_detail()));
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(get_detail());
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<std::is_convertible_v<_Quantified, bool> && !std::is_same_v<_Quantified, bool> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(get_detail());
	}

	// container accessors
public: // public get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return get_detail().begin();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return get_detail().begin();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return get_detail().end();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return get_detail().end();
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return get_detail().size();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<constraints::is_container_v<_Quantified> && !std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return get_detail().size();
	}

	// pointer accessors
private:
	// Helper for arrow operator
	template <bool IsPointer>
	[[nodiscard]] constexpr auto get_pointer() const noexcept -> std::conditional_t<IsPointer, const _Resource&, const _Resource*>
	{
		if constexpr (IsPointer)
		{
			return get_detail();
		}
		else
		{
			return std::addressof(get_detail());
		}
	}

public: // public set
	template <class _Quantified = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept
	{
		constexpr auto is_pointer = std::is_pointer_v<_Quantified>;
		return const_cast<std::conditional_t<is_pointer, _Resource&, _Resource*>>(get_pointer<is_pointer>());
	}

private: // private set
	template <class _Quantified = _Resource, class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept
	{
		constexpr auto is_pointer = std::is_pointer_v<_Quantified>;
		return const_cast<std::conditional_t<is_pointer, _Resource&, _Resource*>>(get_pointer<is_pointer>());
	}

public: // public get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept
	{
		return get_pointer<std::is_pointer_v<_Quantified>>();
	}

private: // private get
	template <class _Quantified = _Resource, class _TModifier = _GetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept
	{
		return get_pointer<std::is_pointer_v<_Quantified>>();
	}

	// Address accessors
public: // public get
	template <class _TModifier = _GetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr const _Resource* operator&() const& noexcept
	{
		return std::addressof(get_detail());
	}

private: // private get
	template <class _TModifier = _GetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr const _Resource* operator&() const& noexcept
	{
		return std::addressof(get_detail());
	}

public: // public set
	template <class _TModifier = _SetterAccess, std::enable_if_t<std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr _Resource* operator&() & noexcept
	{
		return std::addressof(const_cast<_Resource&>(get_detail()));
	}

private: // private set
	template <class _TModifier = _SetterAccess, std::enable_if_t<!std::is_same_v<_TModifier, public_access>, int> = 0>
	[[nodiscard]] constexpr _Resource* operator&() & noexcept
	{
		return std::addressof(const_cast<_Resource&>(get_detail()));
	}

protected:
	_Resource _resource = {};
	getter _getter;
	setter _setter;
};

} // namespace janecekvit::extensions