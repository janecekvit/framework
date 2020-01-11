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
#include "Framework/Extensions/Extensions.h"

namespace Extensions
{

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
	virtual ~ResourceWrapper() = default;
	constexpr ResourceWrapper() = delete;

	constexpr ResourceWrapper(TDeleter&& fnDeleter)
		: GetterSetter<TResource>()
		, m_pDeleter(_CreateDeleter(std::forward<TDeleter>(fnDeleter)))
	{
		
	}

	constexpr ResourceWrapper(TResource&& oResource, TDeleter&& fnDeleter)
		: GetterSetter<TResource>(std::move(oResource))
		, m_pDeleter(_CreateDeleter(std::forward<TDeleter>(fnDeleter)))
	{
	}

	constexpr ResourceWrapper(const TResource& oResource, TDeleter&& fnDeleter)
		: GetterSetter<TResource>(oResource)
		, m_pDeleter(_CreateDeleter(std::forward<TDeleter>(fnDeleter)))
	{
	}

	constexpr ResourceWrapper(const ResourceWrapper& oOther)
		requires std::is_copy_constructible_v<TResource>
		: GetterSetter<TResource>(oOther.m_oResource)
		, m_pDeleter(oOther.m_pDeleter)
	{
	}

	constexpr ResourceWrapper(const ResourceWrapper&)
		requires !std::is_copy_constructible_v<TResource> = delete;

	constexpr ResourceWrapper(ResourceWrapper&& oOther) noexcept
		: GetterSetter<TResource>(std::move(oOther.m_oResource))
		, m_pDeleter(std::move(oOther.m_pDeleter))
	{
	}

	constexpr ResourceWrapper& operator=(const ResourceWrapper& oOther)
		requires std::is_copy_constructible_v<TResource>
	{
		m_pDeleter = oOther.m_pDeleter;
		GetterSetter<TResource>::m_oResource = oOther.m_oResource;
		return *this;
	}

	constexpr ResourceWrapper& operator=(const ResourceWrapper&)
		requires !std::is_copy_constructible_v<TResource> = delete;

	ResourceWrapper& operator=(ResourceWrapper&& oOther) noexcept
	{
		m_pDeleter = std::move(oOther.m_pDeleter);
		GetterSetter<TResource>::m_oResource = std::move(oOther.m_oResource);
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
		(*m_pDeleter)(*this);
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
	constexpr std::shared_ptr<TDeleter> _CreateDeleter(TDeleter&& fnDeleter) 
	{
		return std::shared_ptr<TDeleter>(new TDeleter(std::forward<TDeleter>(fnDeleter)), [this](TDeleter* pFunc)
		{
			//Call inner resource deleter
			(*pFunc)(*this);

			delete pFunc;
			pFunc = nullptr;
		});
	}

protected:
	std::shared_ptr<TDeleter> m_pDeleter = nullptr;
};

} //namespace Extensions
