#pragma once

#include "extensions/constraints.h"

#include <any>
#include <list>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace janecekvit::storage
{

/// <summary>
/// parameter pack class can forward input Variadic argument's list to the any object for future processing
/// parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by out parameters for C++14 and below
/// Packed parameters can be retrieved from pack by return value through std::tuple for C++17 and above
/// </summary>
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>

class parameter_pack
{
public:
	using parameters = std::list<std::any>;

	parameter_pack() = default;
	virtual ~parameter_pack() = default;

	template <class... _Args>
	constexpr parameter_pack(_Args&&... args)
	{
		_insert(std::forward<_Args>(args)...);
	}

	template <class... _Args>
	[[nodiscard]] constexpr std::tuple<_Args...> get_pack() const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto it = _arguments.begin();
		return _deserialize<_Args...>(it);
	}

	[[nodiscard]] size_t size() const noexcept
	{
		return _arguments.size();
	}

protected:
	template <class... _Args>
	constexpr void _insert(_Args&&... args)
	{
		auto process = [&](auto&& value)
		{
			using type = std::decay_t<decltype(value)>;
			if constexpr (constraints::is_tuple_v<type>)
			{
				std::apply([&](auto&&... tuple_args)
					{
						_insert(std::forward<decltype(tuple_args)>(tuple_args)...);
					},
					value);
			}
			else if constexpr (constraints::is_initializer_list_v<type>)
			{
				for (auto&& item : value)
					_insert(std::forward<decltype(item)>(item));
			}
			else
			{
				_arguments.emplace_back(std::make_any<type>(std::forward<decltype(value)>(value)));
			}
		};

		(process(std::forward<_Args>(args)), ...);
	}

	template <class _T, class... _Rest>
	[[nodiscard]] constexpr std::tuple<_T, _Rest...> _deserialize(typename parameters::const_iterator& it) const
	{
		try
		{
			_T oValue = std::any_cast<_T>(*it);
			++it;

			if constexpr (sizeof...(_Rest) > 0)
				return std::tuple_cat(std::make_tuple(oValue), _deserialize<_Rest...>(it));
			else
				return std::make_tuple(oValue);
		}
		catch (const std::bad_any_cast& ex)
		{
			throw std::invalid_argument("Wrong input type: " + std::string(ex.what()));
		}
	}

private:
	parameters _arguments;
};

#if defined(__legacy)

class parameter_pack_legacy
{
private:
	// Helper base class used as wrapper to hold any type described by derived parameter<_T> class
	class parameter_base
	{
	public:
		virtual ~parameter_base() = default;

		template <class _T>
		const _T& get() const; // Method is implemented after parameter derived class, to gain ability to operate with it
	};

	// Helper derived class used as container of any input type.
	template <class _T>
	class parameter : public virtual parameter_base
	{
	public:
		virtual ~parameter() = default;

		parameter(const _T& oValue)
			: m_oValue(oValue)
		{
		}

		constexpr const _T& get() const
		{
			return m_oValue;
		}

	protected:
		_T m_oValue = {};
	};

	// Main class
public:
	using Parameters = std::list<std::shared_ptr<parameter_base>>;
	parameter_pack_legacy() = default;
	virtual ~parameter_pack_legacy() = default;

	template <class... _Args>
	parameter_pack_legacy(
		const _Args&... args)
	{
		_serialize(args...);
	}

public:
	template <class... _Args>
	constexpr void get_pack(
		_Args&... args) const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto tempArgs = _arguments;
		_deserialize(tempArgs, args...);
	}

protected:
	template <class _T>
	constexpr void _serialize(
		const _T& first)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		_arguments.emplace_back(std::make_shared<parameter<_T>>(first));
	}

	template <class _T, class... _Rest>
	constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		_serialize(first);
		_serialize(rest...);
	}

	template <class _T>
	constexpr void _deserialize(
		Parameters& args,
		_T& first) const
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		first = args.front()->get<_T>();
		args.pop_front();
	}

	template <class _T, class... _Rest>
	constexpr void _deserialize(
		Parameters& args,
		_T& first,
		_Rest&... rest) const
	{
		_deserialize(args, first);
		_deserialize(args, rest...);
	}

protected:
	Parameters _arguments;
};

// Core method: create dynamic_cast instead of virtual cast to get type what allocated in derived class
template <class _T>
const _T& storage::parameter_pack_legacy::parameter_base::get() const
{
	using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<parameter<_T>>().get())>::type>::type;
	static_assert(std::is_same<_T, TRetrievedType>::value, "Cannot cast templated return type <_T> to the derived class \"parameter.<_T>Get()\" type!");

	return dynamic_cast<const parameter<_T>&>(*this).get();
}

#endif // __cpp_lib_any

} // namespace janecekvit::storage
