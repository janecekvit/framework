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
{
public:
	class DeleterMissingException : public std::exception
	{
	public:
		DeleterMissingException(const std::type_info& typeInfo) noexcept
		{
			using namespace std::string_literals;
			m_sData = "ResourceWrapper: Deleter with specified type cannot be found: "s + typeInfo.name();
		}
		const char* what() const override
		{
			return m_sData.c_str();
		}
	
	protected:
		std::string m_sData;
	};

public:
	using TAccessor = typename std::function<void(TResource&)>;
	using TConstAccessor = typename std::function<void(const TResource&)>;
	using TDeleter = typename TAccessor;

public:
	virtual ~ResourceWrapper() = default;
	constexpr ResourceWrapper() = delete;

	constexpr ResourceWrapper(TDeleter&& fnDeleter)
		: m_pResource(_CreateResource(TResource{}, TDeleter(fnDeleter)))
		, m_oDeleter(std::forward<TDeleter>(fnDeleter))
	{
	}

	template <class TFwdResource>
	constexpr ResourceWrapper(TFwdResource&& oResource, TDeleter&& fnDeleter)
		: m_pResource(_CreateResource(std::forward<TResource>(oResource), TDeleter(fnDeleter)))
		, m_oDeleter(std::forward<TDeleter>(fnDeleter))
	{
	}

	constexpr ResourceWrapper(const TResource& oResource, TDeleter&& fnDeleter)
		: m_pResource(_CreateResource(oResource, TDeleter(fnDeleter)))
		, m_oDeleter(std::forward<TDeleter>(fnDeleter))
	{
	}

	constexpr ResourceWrapper(const ResourceWrapper& oOther)
		requires std::is_copy_constructible_v<TResource>
		: m_pResource(oOther.m_pResource)
		, m_oDeleter(oOther.m_oDeleter)
	{
	}

	constexpr ResourceWrapper(const ResourceWrapper&)
		requires !std::is_copy_constructible_v<TResource> = delete;

	constexpr ResourceWrapper(ResourceWrapper&& oOther) noexcept
		: m_pResource(std::move(oOther.m_pResource))
		, m_oDeleter(std::move(oOther.m_oDeleter))
	{
	}

	constexpr ResourceWrapper& operator=(const ResourceWrapper& oOther)
		requires std::is_copy_constructible_v<TResource>
	{
		m_oDeleter = oOther.m_oDeleter;
		m_pResource = oOther.m_pResource;
		return *this;
	}

	constexpr ResourceWrapper& operator=(const ResourceWrapper&)
		requires !std::is_copy_constructible_v<TResource> = delete;

	constexpr ResourceWrapper& operator=(ResourceWrapper&& oOther) noexcept
	{
		m_oDeleter = std::move(oOther.m_oDeleter);
		m_pResource = std::move(oOther.m_pResource);
		return *this;
	}

	constexpr ResourceWrapper& operator=(const TResource& oResource)
	{
		m_pResource = _CreateResource(oResource, TDeleter(m_oDeleter));
		return *this;
	}

	constexpr ResourceWrapper& operator=(TResource&& oResource)
	{
		m_pResource = _CreateResource(std::move(oResource), TDeleter(m_oDeleter));
		return *this;
	}

	constexpr void Reset()
	{
		m_pResource = _CreateResource(nullptr, TDeleter(m_oDeleter));
	}

	constexpr void Retrieve(TConstAccessor&& fnAccess) const
	{
		fnAccess(*m_pResource);
	}

	constexpr void Update(TAccessor&& fnAccess)
	{
		fnAccess(*m_pResource);
	}

	[[nodiscard]]
	constexpr operator auto() const& -> const TResource&
	{
		return *m_pResource;
	}

	[[nodiscard]]
	constexpr operator auto() & -> TResource&
	{
		return *m_pResource;
	}

	[[nodiscard]]
	constexpr operator auto() && -> TResource&&
	{
		return std::move(*m_pResource);
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_explicitly_convertible_v<TQuantified, bool>, int> = 0>
	[[nodiscard]]
	constexpr explicit operator bool() noexcept
	{
		return static_cast<bool>(*m_pResource);
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) begin() noexcept
	{
		return m_pResource->begin();
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) end() noexcept
	{
		return m_pResource->end();
	}

	template<class TQuantified = TResource, std::enable_if_t<Constraints::is_container_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr decltype(auto) size() noexcept
	{
		return m_pResource->size();
	}

	template<class TQuantified = TResource, std::enable_if_t<std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & -> TResource&
	{
		return *m_pResource;
	}

	template<class TQuantified = TResource, std::enable_if_t<!std::is_pointer_v<TQuantified>, int> = 0>
	[[nodiscard]]
	constexpr auto operator->() & -> TResource*
	{
		return std::addressof(*m_pResource);
	}
	
	[[nodiscard]]
	constexpr TResource* operator&()
	{
		return std::addressof(*m_pResource);
	}

protected:
	[[nodiscard]]
	constexpr std::shared_ptr<TResource> _CreateResource(TResource&& oResource, TDeleter &&fnDeleter)
	{
		return std::shared_ptr<TResource>(new TResource(std::forward<TResource>(oResource)), [fnStoredDeleter = std::move(fnDeleter)](TResource* pResource)
		{
			//Call inner resource deleter
			if (fnStoredDeleter)
				fnStoredDeleter(*pResource);

			delete pResource;
			pResource = nullptr;

			if (!fnStoredDeleter)
				throw DeleterMissingException(typeid(fnStoredDeleter));
		});
	}

protected:
	std::shared_ptr<TResource> m_pResource {};
	TDeleter m_oDeleter {};
};

} //namespace Extensions
