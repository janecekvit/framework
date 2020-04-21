#pragma once

/*
Copyright (c) 2020 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

GetterSetter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.05 21/04/2020
*/

#include "Framework/Extensions/Constraints.h"

namespace Extensions
{

template <class TResource>
class GetterSetter
{
public:
	constexpr GetterSetter(TResource&& oResource)
		: m_oResource(std::move(oResource))
	{
	}

	constexpr GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

	constexpr GetterSetter() = default;
	virtual ~GetterSetter() = default;
	GetterSetter(const GetterSetter&) = default;
	GetterSetter& operator=(const GetterSetter&) = default;
	GetterSetter(GetterSetter&&) = default;
	GetterSetter& operator=(GetterSetter&&) = default;

	[[nodiscard]]
	constexpr operator auto() const& -> const TResource&
	{
		return m_oResource;
	}

	[[nodiscard]]
	constexpr operator auto() & -> TResource&
	{
		return m_oResource;
	}

	[[nodiscard]]
	constexpr operator auto() && -> TResource&&
	{
		return std::move(m_oResource);
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_explicitly_convertible_v<TQuantified, bool>, int> = 0>
	[[nodiscard]]
	constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(m_oResource);
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() const noexcept
	{
		return m_oResource.begin();
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() const noexcept
	{
		return m_oResource.end();
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() const noexcept
	{
		return m_oResource.size();
	}

	template<class TQuantified = TResource, std::enable_if_t<std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource&
	{
		return m_oResource;
	}

	template<class TQuantified = TResource, std::enable_if_t<std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const & noexcept -> const TResource&
	{
		return m_oResource;
	}

	template<class TQuantified = TResource, std::enable_if_t<!std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & noexcept -> TResource*
	{
		return std::addressof(m_oResource);
	}

	template<class TQuantified = TResource, std::enable_if_t<!std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() const & noexcept  -> const TResource*
	{
		return std::addressof(m_oResource);
	}

	constexpr TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

protected:
	TResource m_oResource = {};
};

} //namespace Extensions