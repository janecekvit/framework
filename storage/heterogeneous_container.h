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
/// <exception cref="bad_access">When cast to the input type failed.</exception>
/// </summary>
class heterogeneous_container final
{
public:
	class bad_access : public std::exception
	{
	public:
		bad_access(const std::type_info& typeInfo, const std::string& error)
			: _typeInfo(typeInfo)
			, _message("heterogeneous_container: " + error + " with type: " + typeInfo.name())
		{
		}

		bad_access(const std::type_info& typeInfo, const std::bad_any_cast& ex)
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

private:
	template <typename _T>
	inline static size_t TypeKey = reinterpret_cast<size_t>(&TypeKey<_T>);

	struct CustomHasher
	{
		std::size_t operator()(size_t key) const noexcept
		{
			return key ^ (key >> 16);
		}
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

	template <class _T = void>
	constexpr void clear()
	{
		m_umapArgs[TypeKey<_T>].clear();
		if constexpr (std::is_same_v<_T, void>)
			m_umapArgs.clear();
	}

public:
	template <class _T = void>
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		if constexpr (std::is_same_v<_T, void>)
			return m_umapArgs.size();

		return m_umapArgs[TypeKey<_T>].size();
	}

	template <class _T = void>
	[[nodiscard]] constexpr bool empty() const noexcept
	{
		if constexpr (std::is_same_v<_T, void>)
			return size() == 0;

		return size<_T>() == 0;
	}

	template <class _T>
	[[nodiscard]] constexpr bool contains() const noexcept
	{
		return size<_T>() > 0;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) first() const
	{
		return get<_T>(0);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get() const
	{
		return _get<_T, true>();
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get()
	{
		return _get<_T, false>();
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t position) const
	{
		try
		{
			auto it = m_umapArgs.find(TypeKey<_T>);
			if (it == m_umapArgs.end() || it->second.size() <= position)
				throw bad_access(typeid(_T), "Cannot retrieve value on position " + std::to_string(position));

			return std::any_cast<const _T&>(it->second[position]);
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		};
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t position)
	{
		return const_cast<_T&>(std::as_const(*this).get<_T>(position));
	}

	template <class _T, class _Callable>
	constexpr void visit(_Callable&& fnCallback)
	{
		_visit<_T, false>(std::forward<_Callable>(fnCallback));
	}

	template <class _T, class _Callable>
	constexpr void visit(_Callable&& fnCallback) const
	{
		_visit<_T, true>(std::forward<_Callable>(fnCallback));
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call_first(_Args&&... args) const
	{
		auto&& oFunc = first<_Func>();
		return std::invoke(oFunc, std::forward<_Args>(args)...);
	}

	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
	constexpr decltype(auto) call(size_t uPosition, _Args&&... args) const
	{
		auto&& oFunc = get<_Func>(uPosition);
		return std::invoke(oFunc, std::forward<_Args>(args)...);
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

private:
	template <class... _Args>
	constexpr void _insert(_Args&&... args)
	{
		auto process = [&](auto&& value)
		{
			using _T = std::decay_t<decltype(value)>;
			if constexpr (constraints::is_tuple_v<_T>)
			{
				std::apply([&](auto&&... tupleArgs)
					{
						_insert(std::forward<decltype(tupleArgs)>(tupleArgs)...);
					},
					value);
			}
			else if constexpr (constraints::is_initializer_list_v<_T>)
			{
				for (auto&& item : value)
					_insert(std::forward<decltype(item)>(item));
			}
			else
			{
				m_umapArgs[TypeKey<_T>].emplace_back(std::make_any<_T>(std::forward<decltype(value)>(value)));
			}
		};

		(process(std::forward<_Args>(args)), ...);
	}

	template <class _T, bool _IsConst>
	[[nodiscard]] constexpr decltype(auto) _get() const
	{
		try
		{
			using Value = std::conditional_t<_IsConst, const _T, _T>;
			std::list<std::reference_wrapper<Value>> values = {};
			for (auto&& item : m_umapArgs[TypeKey<_T>])
				values.emplace_back(std::any_cast<Value&>(item));
			return values;
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		};
	}

	template <class _T, bool _IsConst, class _Callable>
	constexpr void _visit(_Callable&& fnCallback) const
	{
		try
		{
			auto it = m_umapArgs.find(TypeKey<_T>);
			if (it == m_umapArgs.end())
				return;

			for (auto&& item : it->second)
			{
				if constexpr (_IsConst)
					std::invoke(std::forward<_Callable>(fnCallback), std::any_cast<const _T&>(item));
				else
					std::invoke(std::forward<_Callable>(fnCallback), std::any_cast<_T&>(item));
			}
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		}
	}

protected:
	mutable std::unordered_map<size_t, std::vector<std::any>, CustomHasher> m_umapArgs;
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