#pragma once

/*
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

GetterSetter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 05/12/2019
*/

template <class TResource>
class GetterSetter
{
public:
	GetterSetter(TResource && oResource)
		: m_oResource(std::move(oResource))
	{
	}

	GetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

	GetterSetter() = default;
	virtual ~GetterSetter() = default;
	GetterSetter(const GetterSetter&) = default;
	GetterSetter& operator=(const GetterSetter&) = default;
	GetterSetter(GetterSetter&&) = default;
	GetterSetter& operator=(GetterSetter&&) = default;

	operator auto() const& -> const TResource&
	{
		return m_oResource;
	}

	operator auto() & -> TResource&
	{
		return m_oResource;
	}

	operator auto() && -> TResource&&
	{
		return std::move(m_oResource);
	}

	auto operator->() & -> TResource*
	{
		return std::addressof(m_oResource);
	}

	TResource* operator&()
	{
		return std::addressof(m_oResource);
	}

protected:
	TResource m_oResource = {};
};