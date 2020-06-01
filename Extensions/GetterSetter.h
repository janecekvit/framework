#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

GetterSetter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.09 01/06/2020
*/

#include "Framework/Extensions/Constraints.h"

namespace Extensions
{
/// <summary>
/// Default setter that says that GetterSetter wrapper is public.
/// </summary>
class DefaultSetter
{
};

/// <summary>
/// Default getter that says that GetterSetter wrapper is public.
/// </summary>
class DefaultGetter
{
};

/// <summary>
/// The Getter/Setter wrapper implements C++ like assessors to modify inner value of the defined resource type (TResource).
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
/// 	Extensions::GetterSetter<int> Int;
/// 	Extensions::GetterSetter<int, TestGetterSetter> IntSetterPrivate;
/// 	Extensions::GetterSetter<int, TestGetterSetter, TestGetterSetter> IntBothPrivate;
///
/// 	Extensions::GetterSetter<std::vector<int>> Vec;
/// 	Extensions::GetterSetter<std::vector<int>, TestGetterSetter> VecSetterPrivate;
/// 	Extensions::GetterSetter<std::vector<int>, TestGetterSetter, TestGetterSetter> VecBothPrivate;
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
template <class TResource, class TSetter = DefaultSetter, class TGetter = DefaultGetter>
class GetterSetter
{
	friend TGetter;
	friend TSetter;

public:
	constexpr GetterSetter() = default;
	virtual ~GetterSetter()	 = default;

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(TResource&& oResource)
		: m_oResource(std::move(oResource))
	{
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(TResource&& oResource)
		: m_oResource(std::move(oResource))
	{
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter(const GetterSetter& oOther)
		: m_oResource(oOther.m_oResource)
	{
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter(const GetterSetter& oOther)
		: m_oResource(oOther.m_oResource)
	{
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter(GetterSetter&& oOther)
		: m_oResource(std::move(oOther.m_oResource))
	{
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter(GetterSetter&& oOther)
		: m_oResource(std::move(oOther.m_oResource))
	{
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter& operator=(const GetterSetter& oOther)
	{
		m_oResource = oOther.m_oResource;
		return *this;
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter& operator=(const GetterSetter& oOther)
	{
		m_oResource = oOther.m_oResource;
		return *this;
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter& operator=(GetterSetter&& oOther)
	{
		m_oResource = std::move(oOther.m_oResource);
		return *this;
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	GetterSetter& operator=(GetterSetter&& oOther)
	{
		m_oResource = std::move(oOther.m_oResource);
		return *this;
	}

public: // public get
	template <class TModifier = TGetter, std::enable_if_t<std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() const& noexcept -> const TResource&
	{
		return m_oResource;
	}

private: // private get
	template <class TModifier = TGetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() const& noexcept -> const TResource&
	{
		return m_oResource;
	}

public: // public get
	template <class TModifier = TGetter, std::enable_if_t<std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() & noexcept -> const TResource&
	{
		return m_oResource;
	}

private: // private get
	template <class TModifier = TGetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() & noexcept -> const TResource&
	{
		return m_oResource;
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() & noexcept -> TResource&
	{
		return m_oResource;
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() & noexcept -> TResource&
	{
		return m_oResource;
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() && noexcept -> TResource&&
	{
		return std::move(m_oResource);
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr operator auto() && noexcept -> TResource&&
	{
		return std::move(m_oResource);
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_convertible_v<TQuantified, bool> && !std::is_same_v<TQuantified, bool> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(m_oResource);
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto()
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_convertible_v<TQuantified, bool> && !std::is_same_v<TQuantified, bool> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(m_oResource);
	}

public: // public get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto() + we can handle non-cost variable because bool is captured by value
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_convertible_v<TQuantified, bool> && !std::is_same_v<TQuantified, bool> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() noexcept
	{
		return static_cast<bool>(m_oResource);
	}

private: // private get -> bool operator const: cannot be called for native bool to avoid interfering with operator auto() + we can handle non-cost variable because bool is captured by value
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_convertible_v<TQuantified, bool> && !std::is_same_v<TQuantified, bool> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr explicit operator bool() noexcept
	{
		return static_cast<bool>(m_oResource);
	}

public: // public get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return m_oResource.begin();
	}

private: // private get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) begin() const noexcept
	{
		return m_oResource.begin();
	}

public: // public get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return m_oResource.end();
	}

private: // private get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) end() const noexcept
	{
		return m_oResource.end();
	}

public: // public get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return m_oResource.size();
	}

private: // private get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr decltype(auto) size() const noexcept
	{
		return m_oResource.size();
	}

public: // public set
	template <class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> TResource&
	{
		return m_oResource;
	}

private: // private set
	template <class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> TResource&
	{
		return m_oResource;
	}

public: // public get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const TResource&
	{
		return m_oResource;
	}

private: // private get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const TResource&
	{
		return m_oResource;
	}

public: // public set
	template <class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> TResource*
	{
		return std::addressof(m_oResource);
	}

private: // private set
	template <class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() & noexcept -> TResource*
	{
		return std::addressof(m_oResource);
	}

public: // public get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const TResource*
	{
		return std::addressof(m_oResource);
	}

private: // private get
	template <class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]] constexpr auto operator->() const& noexcept -> const TResource*
	{
		return std::addressof(m_oResource);
	}

public: // public set
	template <class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

private: // private set
	template <class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

protected:
	TResource m_oResource = {};
};

} //namespace Extensions