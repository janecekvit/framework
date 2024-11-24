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

public:
	~heterogeneous_container() = default;

	heterogeneous_container() noexcept = default;

	template <class... _Args>
	constexpr heterogeneous_container(
		const _Args&... args)
	{
		_serialize(args...);
	}

	template <class... _Args>
	constexpr void emplace(const _Args&... args)
	{
		_serialize(args...);
	}

	template <class _T>
	constexpr void reset()
	{
		extensions::execute_on_container(m_umapArgs, std::type_index(typeid(_T)), [](std::list<std::any>& listInput)
			{
				listInput.clear();
			});
	}

	void reset()
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
		return extensions::execute_on_container(m_umapArgs, std::type_index(typeid(_T)), [](const std::list<std::any>& listInput)
			{
				return listInput.size();
			});
	}

	template <class _T>
	[[nodiscard]] constexpr bool contains() const noexcept
	{
		return size<_T>() > 0;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get() const
	{
		return _deserialize<_T>();
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t uPosition) const
	{
		return _deserialize<_T>(uPosition);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) first() const
	{
		return _deserialize<_T>(0);
	}

	template <class _T>
	constexpr void visit(std::function<void(const _T&)>&& fnCallback) const
	{
		_visit<_T>(std::move(fnCallback));
	}

	template <class _T>
	constexpr void visit(std::function<void(_T&)>&& fnCallback)
	{
		_visit<_T>(std::move(fnCallback));
	}

private:
	template <class _T, class... _Rest>
	constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		m_umapArgs[std::type_index(typeid(_T))].emplace_back(std::make_any<_T>(first));

		if constexpr (sizeof...(_Rest) > 0)
			_serialize(rest...);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) _deserialize() const
	{
		std::list<_T> oList = {};
		_visit<_T>([&oList](const _T& input)
			{
				oList.emplace_back(input);
			});
		return oList;
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) _deserialize(size_t uPosition) const
	{
		size_t uCounter = 0;
		std::optional<_T> oValue = std::nullopt;
		_visit<_T>([&](const _T& input)
			{
				if (uCounter == uPosition)
					oValue = std::make_optional<_T>(input);
				uCounter++;
			});

		if (!oValue)
			throw heterogeneous_container_exception(typeid(_T), "Cannot retrieve value on position " + std::to_string(uPosition));
		return static_cast<_T>(std::move(oValue.value()));
	}

	template <class _T>
	constexpr void _visit(std::function<void(_T&)>&& fnCallback) const
	{
		try
		{
			extensions::execute_on_container(m_umapArgs, std::type_index(typeid(_T)), [&fnCallback](std::list<std::any>& listInput)
				{
					for (auto&& item : listInput)
						fnCallback(std::any_cast<_T&>(item));
				});
		}
		catch (const std::bad_any_cast& ex)
		{
			throw heterogeneous_container_exception(typeid(_T), ex);
		};
	}

protected:
	mutable std::unordered_map<std::type_index, std::list<std::any>> m_umapArgs;
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