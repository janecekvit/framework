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

/// <summary>
/// The wrapper implemented own deleter that gains functionality to release all used resources correctly.
/// Method derives from the GetterSetter class to using input resource with implicit conversions.
/// Ensure that deleter's body can be called multiple times to handle deallocations of the same resource in case of CopyConstructible methods of this wrapper are used.
/// </summary>
/// <example>
/// <code>
/// auto oWrapperFile = ResourceWrapper<std::fstream>(std::fstream("Test", std::ios::binary), [](std::fstream& i)
/// {
///		i.close();
/// });
///	bool open = oWrapperFile->is_open();
/// </code>
/// </example>
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

	ResourceWrapper(const ResourceWrapper& oOther)
	{
		ResourceWrapper::operator=(oOther);
	}

	ResourceWrapper(ResourceWrapper&& oOther)
	{
		ResourceWrapper::operator=(std::move(oOther));
	}

	ResourceWrapper& operator=(const ResourceWrapper& oOther)
	{
		ResourceWrapper::operator=(oOther.m_oResource);
		m_fnDeleter = oOther.m_fnDeleter;
		return *this;
	}

	ResourceWrapper& operator=(ResourceWrapper&& oOther)
	{
		ResourceWrapper::operator=(std::move(oOther.m_oResource));
		m_fnDeleter = std::move(oOther.m_fnDeleter);
		return *this;
	}

	ResourceWrapper& operator=(const TResource& oResource)
	{
		Reset();
		GetterSetter<TResource>::m_oResource = oResource;
		return *this;
	}

	ResourceWrapper& operator=(TResource&& oResource)
	{
		Reset();
		GetterSetter<TResource>::m_oResource = std::move(oResource);
		return *this;
	}

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