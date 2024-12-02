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
/// </summary>
template <class... _UserDefinedTypes>
class heterogeneous_container final
{
public:
	using DefaultKnownTypes = std::variant<
		bool, short, unsigned short, int, unsigned int, long, unsigned long, float, double, size_t, std::byte,
		char, char*, const char*, wchar_t, wchar_t*, const wchar_t*, std::string, std::wstring, std::u8string, std::u16string, std::u32string>;

	using KnownTypes = std::conditional_t<
		sizeof...(_UserDefinedTypes) == 0,
		DefaultKnownTypes,
		constraints::unify_variant_t<DefaultKnownTypes, std::variant<_UserDefinedTypes...>>>;

	template <typename _T>
	static constexpr bool IsKnownType = constraints::is_type_in_variant_v<_T, KnownTypes>;

public:
	class bad_access : public std::exception
	{
	public:
		bad_access(const std::type_info& typeInfo, const std::string& error)
			: _typeInfo(typeInfo)
			, _message("heterogeneous_container: " + error + " with type: " + typeInfo.name())
		{
		}

		bad_access(const std::type_info& typeInfo, const std::exception& ex)
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
	class item
	{
	public:
		template <typename _T, std::enable_if_t<constraints::is_type_in_variant_v<std::decay_t<_T>, KnownTypes>, int> = 0>
		item(_T&& value)
			: _value(std::in_place_type<KnownTypes>, KnownTypes(std::in_place_type<std::decay_t<_T>>, std::forward<_T>(value)))
		{
		}

		template <typename _T, std::enable_if_t<!constraints::is_type_in_variant_v<std::decay_t<_T>, KnownTypes>, int> = 0>
		item(_T&& value)
			: _value(std::in_place_type<std::any>, std::make_any<_T>(std::forward<_T>(value)))
		{
		}

		template <typename _T>
		constexpr auto& get()
		{
			return const_cast<_T&>(std::as_const(*this).get<_T>());
		}

		template <typename _T>
		constexpr const auto& get() const
		{
			if constexpr (heterogeneous_container::IsKnownType<_T>)
				return std::as_const(std::get<_T>(std::get<KnownTypes>(_value)));
			else
				return std::any_cast<const _T&>(std::get<std::any>(_value));
		}

	private:
		std::variant<KnownTypes, std::any> _value;
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
		if constexpr (std::is_same_v<_T, void>)
			_values.clear();
		else
			_get_storage<_T>().clear();
	}

public:
	template <class _T = void>
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		if constexpr (!std::is_same_v<_T, void>)
			return _get_storage<_T>().size();

		return _values.size();
	}

	template <class _T = void>
	[[nodiscard]] constexpr bool empty() const noexcept
	{
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
	[[nodiscard]] constexpr decltype(auto) first()
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
		return _get<_T>(position);
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) get(size_t position)
	{
		return const_cast<_T&>(std::as_const(*this)._get<_T>(position));
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

#ifdef __cpp_lib_concepts
	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
#else
	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
#endif // __cpp_lib_concepts
	constexpr decltype(auto) call_first(_Args&&... args) const
	{
		auto&& oFunc = first<_Func>();
		return std::invoke(oFunc, std::forward<_Args>(args)...);
	}

#ifdef __cpp_lib_concepts
	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
#else
	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
#endif // __cpp_lib_concepts
	constexpr decltype(auto) call(size_t uPosition, _Args&&... args) const
	{
		auto&& oFunc = get<_Func>(uPosition);
		return std::invoke(oFunc, std::forward<_Args>(args)...);
	}

#ifdef __cpp_lib_concepts
	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
#else
	template <class _Func, class... _Args, std::enable_if_t<std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>, int> = 0>
#endif // __cpp_lib_concepts
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
				_values[TypeKey<_T>].emplace_back(std::forward<decltype(value)>(value));
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

			auto& storage = _get_storage_by_find<_T>();
			for (auto&& item : storage)
				values.emplace_back(item.get<_T>());

			return values;
		}
		catch (const std::bad_variant_access& ex)
		{
			throw bad_access(typeid(_T), ex);
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		};
	}

	template <class _T>
	[[nodiscard]] constexpr decltype(auto) _get(size_t position) const
	{
		try
		{
			auto& storage = _get_storage_by_find<_T>();
			if (storage.size() <= position)
				throw bad_access(typeid(_T), "Cannot retrieve value on position " + std::to_string(position));

			return storage[position].get<_T>();
		}
		catch (const std::bad_variant_access& ex)
		{
			throw bad_access(typeid(_T), ex);
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
			auto& storage = _get_storage_by_find<_T>();
			for (auto&& item : storage)
			{
				if constexpr (_IsConst)
					std::invoke(fnCallback, std::as_const(item.get<_T>()));
				else
					std::invoke(fnCallback, item.get<_T>());
			}
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		}
	}

	template <typename _T>
	constexpr auto& _get_storage() const
	{
		return _values[TypeKey<_T>];
	}

	template <typename _T, bool _ThrowException = true>
	constexpr auto& _get_storage_by_find() const
	{
		auto&& it = _values.find(TypeKey<_T>);
		if (it == _values.end())
			throw bad_access(typeid(_T), "Cannot find type in container.");
		return it->second;
	}

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

	mutable std::unordered_map<size_t, std::vector<item>, CustomHasher> _values;
};

} // namespace storage

} // namespace janecekvit