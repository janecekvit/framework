#pragma once

/*
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.
All rights reserved.

ResourceWrapper.h
Purpose:	header file contains RAII pattern 

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 05/12/2019
*/

#include <functional>
#include "Framework/Extensions/GetterSetter.h"


template <class TResource>
class ResourceWrapper 
	: public GetterSetter<TResource>
{
public:
	using TAccessor = typename std::function<void(TResource&)>;
	using TConstAccessor = typename std::function<void(const TResource&)>;
	using TDeleter = typename TAccessor;
public:
	ResourceWrapper(TDeleter&& fnDeleter)
		: GetterSetter<TResource>()
		, m_fnDeleter(std::move(fnDeleter))
	{
	}

	ResourceWrapper(TResource && oResource, TDeleter && fnDeleter)
		: GetterSetter<TResource>(std::move(oResource))
		, m_fnDeleter(std::move(fnDeleter))
	{
	}

	ResourceWrapper(const TResource& oResource, TDeleter&& fnDeleter)
		: GetterSetter<TResource>(oResource)
		, m_fnDeleter(std::move(fnDeleter))
	{
	}

	virtual ~ResourceWrapper()
	{
		try
		{
			Reset();
		}
		catch (...)
		{ // check double exception serious error
		} 
	}

	ResourceWrapper(const ResourceWrapper&) = default;
	ResourceWrapper& operator=(const ResourceWrapper&) = default;
	ResourceWrapper(ResourceWrapper&&) = default;
	ResourceWrapper& operator=(ResourceWrapper&&) = default;

	void Reset()
	{
		m_fnDeleter(*this);
		GetterSetter<TResource>::m_oResource = TResource {};
	}

	void Retrieve(TConstAccessor&& fnAccess) const
	{
		fnAccess(*this);
	}

	void Update(TAccessor&& fnAccess)
	{
		fnAccess(*this);
	}

protected:
	TDeleter m_fnDeleter = nullptr;
};