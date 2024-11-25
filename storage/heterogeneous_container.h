#pragma once

#include "extensions/extensions.h"
#include "synchronization/concurrent.h"

#include <any>
#include <functional>
#include <list>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace janecekvit
{

namespace storage
{

/// <summary>
/// Heterogeneous Container store any copy constructible object for the future processing
///  Heterogeneous Container implement lazy evaluation idiom to enable processing input arguments as late as possible
/// <exception cref="heterogeneous_container_exception">When cast to the input type failed.</exception>
/// </summary>
class heterogeneous_container final
{
public:
	class heterogeneous_container_exception : public std::exception
	{
	public:
		heterogeneous_container_exception(const std::type_info& typeInfo, const std::string& error)
			: _typeInfo(typeInfo)
			, _message("heterogeneous_container: " + error + " with type: " + typeInfo.name())
		{
		}

		heterogeneous_container_exception(const std::type_info& typeInfo, const std::bad_any_cast& ex)
			: _typeInfo(typeInfo)
			, _message("heterogeneous_container: " + std::string(ex.what()) + " with type: " + typeInfo.name())
		{
		}

		const std::type_info& type_info() const noexcept
		{
			return _typeInfo;
		}

		const char* what() const noexcept override
		{
			return _message.c_str();
		}

	private:
		const std::type_info& _typeInfo;
		std::string _message;
	};

	template <typename _T>
	struct TypeKey
	{
		inline static size_t value = []() -> size_t
		{
			static const size_t id = reinterpret_cast<size_t>(&id);
			return id;
		}();
	};

public:
	~heterogeneous_container() = default;

	heterogeneous_container() noexcept = default;

	template <class... _Args>
	constexpr heterogeneous_container(
		const _Args&... args)
	{
		_insert(args...);
	}

	template <class... _Args>
	constexpr void emplace(const _Args&... args)
	{
		_insert(args...);
	}

	template <class _T>
	constexpr void clear()
	{
		m_umapArgs[TypeKey<_T>::value].clear();
	}

	void clear()
	{
		m_umapArgs.clear();
	}

public:
	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call_first(_Args&&... args) const
	{
		auto&& oFunc = first<_Func>();
		return std::invoke(oFunc, std::forward<_Args>(args)...); // NVRO
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call(size_t uPosition, _Args&&... args) const
	{
		auto&& oFunc = get<_Func>(uPosition);
		return std::invoke(oFunc, std::forward<_Args>(args)...); // NVRO
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call_all(_Args&&... args) const
	{
		using RetType = std::invoke_result_t<_Func, _Args...>;
		if constexpr (std::is_void_v<RetType>)
		{
			for (auto&& func : get<_Func>())
				std::invoke(func, std::forward<_Args>(args)...);
		}
		else
		{
			std::list<RetType> oList = {};
			for (auto&& func : get<_Func>())
				oList.emplace_back(std::invoke(func, std::forward<_Args>(args)...));
			return oList;
		}
	}

	template <class _T>
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		return m_umapArgs[TypeKey<_T>::value].size();
	}

	template <class _T>
	[[nodiscard]] constexpr bool contains() const noexcept
	{
		return size<_T>() > 0;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get() const
	{
		std::list<_T> values = {};
		try
		{
			for (auto&& item : m_umapArgs[TypeKey<_T>::value])
				values.emplace_back(std::any_cast<_T&>(item));
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};

		return values;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t position) const
	{
		try
		{
			return std::any_cast<_T&>(m_umapArgs[TypeKey<_T>::value].at(position));
		}
		catch (const std::out_of_range& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), "Cannot retrieve value on position " + std::to_string(position));
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) first() const
	{
		return get<_T>(0);
	}

	template <class _T>
	constexpr void visit(std::function<void(const _T&)>&& fnCallback) const
	{
		try
		{
			for (auto&& item : m_umapArgs[TypeKey<_T>::value])
				fnCallback(std::any_cast<_T&>(item));
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};
	}

	template <class _T>
	constexpr void visit(std::function<void(_T&)>&& fnCallback)
	{
		try
		{
			for (auto&& item : m_umapArgs[TypeKey<_T>::value])
				fnCallback(std::any_cast<_T&>(item));
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};
	}

private:
	template <class... _Args>
	constexpr void _insert(_Args&&... args)
	{
		auto process = [&](auto&& value)
		{
			using _T = std::decay_t<decltype(value)>;
			static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");

			m_umapArgs[TypeKey<_T>::value].emplace_back(std::make_any<_T>(std::forward<decltype(value)>(value)));
		};

		(process(std::forward<_Args>(args)), ...);
	}

protected:
	mutable std::unordered_map<size_t, std::vector<std::any>> m_umapArgs;
};

} // namespace storage

namespace extensions::tuple
{
namespace details
{

template <class _Tuple, std::size_t... _I>
constexpr storage::heterogeneous_container unpack(_Tuple&& t, std::index_sequence<_I...>)
{
	return storage::heterogeneous_container{ std::get<_I>(t)... };
}

} // namespace details

/// <summary>
/// unpack tuple to the Heterogeneous container
/// </summary>
template <class _Tuple>
[[nodiscard]] constexpr storage::heterogeneous_container unpack(_Tuple&& t)
{
	return details::unpack(std::forward<_Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>{});
}

} // namespace extensions::tuple

} // namespace janecekvit