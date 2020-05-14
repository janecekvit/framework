#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

GetterSetter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.07 15/05/2020
*/

#include "Framework/Extensions/Constraints.h"

namespace Extensions
{

class DefaultSetter
{
};

class DefaultGetter
{
};

template <class TResource, class TSetter = DefaultSetter, class TGetter = DefaultGetter>
class GetterSetter
{
	friend TGetter;
	friend TSetter;

public:
	constexpr GetterSetter() = default;
	virtual ~GetterSetter() = default;

public: // public set
	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(TResource&& oResource)
		: m_oResource(std::move(oResource))
	{
	}

private: // private set
	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(TResource&& oResource)
		: m_oResource(std::move(oResource))
	{
	}

public: // public set
	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

private: // private set
	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

//public: // public set	
//	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter(const GetterSetter&) = default;
//
//private: // private set
//	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter(const GetterSetter&) = default;
//
//public: // public set
//	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter(GetterSetter&&) = default;
//
//private: // private set
//	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter(GetterSetter&&) = default;
//
//public: // public set
//	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter& operator=(const GetterSetter&) = default;
//
//private: // private set
//	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter& operator=(const GetterSetter&) = default;
//
//public: // public set
//	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter& operator=(GetterSetter&&) = default;
//
//private: // private set
//	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
//	GetterSetter& operator=(GetterSetter&&) = default;

public: // public get
	template<class TModifier = TGetter, std::enable_if_t<std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() const& -> const TResource&
	{
		return m_oResource;
	}

private: // private get
	template<class TModifier = TGetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() const& -> const TResource&
	{
		return m_oResource;
	}

public: // public set
	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() & -> TResource&
	{
		return m_oResource;
	}

private: // private set
	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() & -> TResource&
	{
		return m_oResource;
	}

public: // public set
	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() && -> TResource&&
	{
		return std::move(m_oResource);
	}

private: // private set
	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr operator auto() && -> TResource&&
	{
		return std::move(m_oResource);
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_explicitly_convertible_v<TQuantified, bool> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(m_oResource);
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_explicitly_convertible_v<TQuantified, bool> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(m_oResource);
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() const noexcept
	{
		return m_oResource.begin();
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() const noexcept
	{
		return m_oResource.begin();
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() const noexcept
	{
		return m_oResource.end();
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified>&& !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() const noexcept
	{
		return m_oResource.end();
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() const noexcept
	{
		return m_oResource.size();
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<Constraints::is_container_v<TQuantified>&& !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() const noexcept
	{
		return m_oResource.size();
	}

public: // public set
	template<class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource&
	{
		return m_oResource;
	}

private: // private set
	template<class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource&
	{
		return m_oResource;
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const & noexcept -> const TResource&
	{
		return m_oResource;
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<std::is_pointer_v<TQuantified>&& !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const& noexcept -> const TResource&
	{
		return m_oResource;
	}

public: // public set
	template<class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource*
	{
		return std::addressof(m_oResource);
	}

private: // private set
	template<class TQuantified = TResource, class TModifier = TSetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource*
	{
		return std::addressof(m_oResource);
	}

public: // public get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const& noexcept  -> const TResource*
	{
		return std::addressof(m_oResource);
	}

private: // private get
	template<class TQuantified = TResource, class TModifier = TGetter, std::enable_if_t<!std::is_pointer_v<TQuantified> && !std::is_same_v<TModifier, DefaultGetter>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const& noexcept  -> const TResource*
	{
		return std::addressof(m_oResource);
	}

public: // public set
	template<class TModifier = TSetter, std::enable_if_t<std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

private: // private set
	template<class TModifier = TSetter, std::enable_if_t<!std::is_same_v<TModifier, DefaultSetter>, int> = 0>
	constexpr TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

protected:
	TResource m_oResource = {};
};

} //namespace Extensions