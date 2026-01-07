#pragma once

#include "exception/exception.h"
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
#include <utility>

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
	using default_known_types = std::variant<
		bool, short, unsigned short, int, unsigned int, long, unsigned long, float, double, size_t, std::byte,
		char, char*, const char*, wchar_t, wchar_t*, const wchar_t*, std::string, std::wstring, std::u8string, std::u16string, std::u32string>;

	using known_types = std::conditional_t<
		sizeof...(_UserDefinedTypes) == 0,
		default_known_types,
		constraints::unify_variant_t<default_known_types, std::variant<_UserDefinedTypes...>>>;

	template <typename _T>
	static constexpr bool is_known_type = constraints::is_type_in_variant_v<_T, known_types>;

public:
	class bad_access : public exception::exception
	{
	public:
		bad_access(const std::type_info& type_info, const std::string& error, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
#if defined(HAS_STD_FORMAT)
			: exception::exception(std::move(srcl), std::move(thread), "heterogeneous_container: {} with type: {}", error, type_info.name())
#else
			: exception::exception("heterogeneous_container: " + error + " with type: " + type_info.name(), std::move(srcl), std::move(thread))
#endif
			, _type_info(type_info)
		{
		}

		bad_access(const std::type_info& type_info, const std::exception& ex, std::source_location&& srcl = std::source_location::current(), std::thread::id&& thread = std::this_thread::get_id())
#if defined(HAS_STD_FORMAT)
			: exception::exception(std::move(srcl), std::move(thread), "heterogeneous_container: {} with type: {}", std::string(ex.what()), type_info.name())
#else
			: exception::exception("heterogeneous_container: " + std::string(ex.what()) + " with type: " + type_info.name(), std::move(srcl), std::move(thread))
#endif
			, _type_info(type_info)
		{
		}

		const std::type_info& type_info() const noexcept
		{
			return _type_info;
		}

	private:
		const std::type_info& _type_info;
	};

public:
	class item
	{
	public:
		template <typename _T, std::enable_if_t<constraints::is_type_in_variant_v<std::decay_t<_T>, known_types>, int> = 0>
		item(_T&& value)
			: _key(TypeKey<std::decay_t<_T>>())
			, _value(std::in_place_type<known_types>, std::in_place_type<std::decay_t<_T>>, std::forward<_T>(value))
		{
		}

		template <typename _T, std::enable_if_t<!constraints::is_type_in_variant_v<std::decay_t<_T>, known_types>, int> = 0>
		item(_T&& value)
			: _key(TypeKey<std::decay_t<_T>>())
			, _value(std::in_place_type<std::any>, std::in_place_type<std::decay_t<_T>>, std::forward<_T>(value))
		{
		}

		template <typename _T>
		[[nodiscard]] constexpr bool is_type() const
		{
			return TypeKey<_T>() == _key;
		}

		template <typename _T>
		constexpr auto& get()
		{
			return const_cast<_T&>(std::as_const(*this).template get<_T>());
		}

		template <typename _T>
		constexpr const auto& get() const
		{
			if constexpr (heterogeneous_container::is_known_type<_T>)
				return std::as_const(std::get<_T>(std::get<known_types>(_value)));
			else
				return std::any_cast<const _T&>(std::get<std::any>(_value));
		}

	private:
		const size_t _key = 0;
		std::variant<known_types, std::any> _value;
	};

private:
	using container = std::unordered_map<size_t, std::vector<item>>;

public:
	template <bool _IsConst>
	class base_iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = item;
		using difference_type = std::ptrdiff_t;
		using pointer = std::conditional_t<_IsConst, const item*, item*>;
		using reference = std::conditional_t<_IsConst, const item&, item&>;

		using container_iterator = std::conditional_t<_IsConst,
			typename container::const_iterator,
			typename container::iterator>;

	public:
		base_iterator(container_iterator it, container_iterator end, size_t index = 0)
			: _map_it(it)
			, _map_end(end)
			, _vector_index(index)
		{
		}

		reference operator*() const noexcept
		{
			return _map_it->second[_vector_index];
		}

		pointer operator->() const noexcept
		{
			return &(_map_it->second[_vector_index]);
		}

		base_iterator& operator++() noexcept
		{
			if (++_vector_index >= _map_it->second.size())
			{
				do
				{
					if (++_map_it == _map_end)
					{
						_vector_index = 0;
						return *this;
					}
				} while (_map_it->second.empty());
				_vector_index = 0;
			}
			return *this;
		}

		base_iterator operator++(int) noexcept
		{
			auto tmp = *this;
			++(*this);
			return tmp;
		}

		bool operator==(const base_iterator& other) const noexcept
		{
			return _map_it == other._map_it && _vector_index == other._vector_index;
		}

		bool operator!=(const base_iterator& other) const noexcept
		{
			return !(*this == other);
		}

	private:
		container_iterator _map_it;
		container_iterator _map_end;
		size_t _vector_index;
	};

	class iterator : public base_iterator<false>
	{
	public:
		using base_iterator<false>::base_iterator;
	};

	class const_iterator : public base_iterator<true>
	{
	public:
		using base_iterator<true>::base_iterator;
	};

public:
	~heterogeneous_container() = default;

	heterogeneous_container() noexcept = default;

	heterogeneous_container(const heterogeneous_container&) = default;

	heterogeneous_container(heterogeneous_container&&) noexcept = default;

	heterogeneous_container& operator=(const heterogeneous_container&) = default;

	heterogeneous_container& operator=(heterogeneous_container&&) noexcept = default;

	template <class... _Args>
	constexpr heterogeneous_container(_Args&&... args)
	{
		_insert(std::forward<_Args>(args)...);
	}

	template <class... _Args>
	constexpr void emplace(_Args&&... args)
	{
		_insert(std::forward<_Args>(args)...);
	}

	template <class _T = void>
	constexpr void clear()
	{
		if constexpr (std::is_same_v<_T, void>)
			_values.clear();
		else
			_get_storage_by_find<_T>().clear();
	}

	template <class _T>
	constexpr void reserve(size_t capacity)
	{
		_values[TypeKey<_T>()].reserve(capacity);
	}

public:
	template <class _T = void>
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		if constexpr (!std::is_same_v<_T, void>)
		{
			auto it = _values.find(TypeKey<_T>());
			return (it != _values.end()) ? it->second.size() : 0;
		}
		else
		{
			size_t total = 0;
			for (const auto& [key, vec] : _values)
				total += vec.size();
			return total;
		}
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
		return _get<_T>(position);
	}

	template <class _T, class _Callable>
	constexpr void visit(_Callable&& callback)
	{
		_visit<_T>(std::forward<_Callable>(callback));
	}

	template <class _T, class _Callable>
	constexpr void visit(_Callable&& callback) const
	{
		_visit<_T>(std::forward<_Callable>(callback));
	}

	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
	constexpr decltype(auto) call_first(_Args&&... args) const
	{
		auto&& oFunc = first<_Func>();
		return std::invoke(oFunc, std::forward<_Args>(args)...);
	}

	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
	constexpr decltype(auto) call(size_t position, _Args&&... args) const
	{
		auto&& oFunc = get<_Func>(position);
		return std::invoke(oFunc, std::forward<_Args>(args)...);
	}

	template <class _Func, class... _Args>
		requires std::is_invocable_r_v<std::invoke_result_t<_Func, _Args...>, _Func, _Args...>
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

	iterator begin() noexcept
	{
		return iterator(_values.begin(), _values.end());
	}

	const_iterator begin() const noexcept
	{
		return const_iterator(_values.cbegin(), _values.cend());
	}

	const_iterator cbegin() const noexcept
	{
		return const_iterator(_values.cbegin(), _values.cend());
	}

	iterator end() noexcept
	{
		return iterator(_values.end(), _values.end());
	}

	const const_iterator end() const noexcept
	{
		return const_iterator(_values.cend(), _values.cend());
	}

	const const_iterator cend() const noexcept
	{
		return const_iterator(_values.cend(), _values.cend());
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
					std::forward<decltype(value)>(value));
			}
			else if constexpr (constraints::is_initializer_list_v<_T>)
			{
				for (auto&& element : value)
					_insert(std::forward<decltype(element)>(element));
			}
			else
			{
				_values[TypeKey<_T>()].emplace_back(std::forward<decltype(value)>(value));
			}
		};

		(process(std::forward<_Args>(args)), ...);
	}

	template <class _T, bool _IsConst>
	[[nodiscard]] constexpr decltype(auto) _get() const
	{
		try
		{
			using _Value = std::conditional_t<_IsConst, const _T, _T>;
			std::list<std::reference_wrapper<_Value>> values = {};

			const auto& storage = _get_storage_by_find<_T>();
			for (const auto& element : storage)
			{
				if constexpr (_IsConst)
					values.emplace_back(element.template get<_T>());
				else
					values.emplace_back(const_cast<_T&>(element.template get<_T>()));
			}

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
			const auto& storage = _get_storage_by_find<_T>();
			if (storage.size() <= position)
				throw bad_access(typeid(_T), "Cannot retrieve value on position " + std::to_string(position));

			return storage[position].template get<_T>();
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
	[[nodiscard]] constexpr decltype(auto) _get(size_t position)
	{
		return const_cast<_T&>(std::as_const(*this).template _get<_T>(position));
	}

	template <class _T, class _Callable>
	constexpr void _visit(const _Callable& callback) const
	{
		try
		{
			const auto& storage = _get_storage_by_find<_T>();
			for (const auto& element : storage)
			{
				std::invoke(callback, std::as_const(element.template get<_T>()));
			}
		}
		catch (const std::bad_any_cast& ex)
		{
			throw bad_access(typeid(_T), ex);
		}
	}

	template <class _T, class _Callable>
	constexpr void _visit(const _Callable& callback)
	{
		std::as_const(*this).template _visit<_T>([&callback](const _T& value)
			{
				callback(const_cast<_T&>(value));
			});
	}

	template <typename _T>
	constexpr const std::vector<item>& _get_storage_by_find() const
	{
		auto it = _values.find(TypeKey<_T>());
		if (it == _values.end())
			throw bad_access(typeid(_T), "Cannot find type in container.");
		return it->second;
	}

	template <typename _T>
	constexpr std::vector<item>& _get_storage_by_find()
	{
		return const_cast<std::vector<item>&>(std::as_const(*this).template _get_storage_by_find<_T>());
	}

private:
	template <typename _T>
	inline static size_t TypeKey() noexcept
	{
		return typeid(_T).hash_code();
	}

	container _values;
};

} // namespace storage

} // namespace janecekvit
