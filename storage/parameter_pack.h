#pragma once

#include <any>
#include <list>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>


namespace janecekvit::storage
{

/// <summary>
/// parameter pack class can forward input variadic argument's list to the any object for future processing
/// parameter pack implement lazy evaluation idiom to enable processing input arguments as late as possible
/// Packed parameters can be retrieved from pack by out parameters for C++14 and below
/// Packed parameters can be retrieved from pack by return value through std::tuple for C++17 and above
/// </summary>
/// <exception cref="std::invalid_argument">When bad number of arguments received in Get methods.</exception>
/// <example>
/// <code>
///
/*
///
/// struct IInterface
/// {
/// 	virtual ~IInterface() = default;
/// 	virtual int Do() = 0;
/// };
///
/// struct CInterface : public virtual IInterface
/// {
/// 	CInterface() = default;
/// 	virtual ~CInterface() = default;
/// 	virtual int Do() final override { return 1111; }
/// };
///
///
/// struct IParamTest
/// {
/// 	virtual ~IParamTest() = default;
/// 	virtual void Run(extensions::Storage::parameter_pack&& oPack) = 0;
/// };
/// struct CParamTest : public virtual IParamTest
/// {
/// 	CParamTest() = default;
/// 	virtual ~CParamTest() = default;
/// 	virtual void Run(extensions::Storage::ParemeterPack22&& oPack) override
/// 	{
///#if __cplusplus >= 201703L
/// 	    int a = 0;
/// 		int b = 0;
/// 		int *c = nullptr;
/// 		std::shared_ptr<int> d = nullptr;
/// 		CInterface* pInt = nullptr;
///
///			//Use unpack by output parameters
/// 		oPack.get_pack(a, b, c, d, pInt);
///
///			//TODO: ANY STUFF
///
///#else
/// 		auto [iNumber1, iNumber2, pNumber, pShared, pInterface] = oPack.get_pack<int, int, int*, std::shared_ptr<int>, CInterface*>();
///
///			//TODO: ANY STUFF
///
///#endif
///		}
/// };
///
///
/// void SomeFunc()
/// {
///		CParamTest oTest;
///
///		int* pInt = new int(666);
///		auto pShared = std::make_shared<int>(777);
///		CInterface oInt;
///
///		// Initialize parameter pack
///		auto oPack = extensions::parameter_pack(25, 333, pInt, pShared, &oInt);
///		oTest.Run(std::move(oPack));
/// }
///
*/
///
/// </code>
/// </example>
#if __cplusplus >= __cpp_lib_any

class parameter_pack
{
public:
	using Parameters = std::list<std::any>;

	parameter_pack()		  = default;
	virtual ~parameter_pack() = default;

	template <class... _Args>
	constexpr parameter_pack(
		const _Args&... args)
	{
		_serialize(args...);
	}

	template <class... _Args>
	[[nodiscard]] constexpr std::tuple<_Args...> get_pack() const
	{
		if (_arguments.size() != sizeof...(_Args))
			throw std::invalid_argument("Bad number of input arguments!");

		auto args = _arguments;
		return _deserialize<_Args...>(std::move(args));
	}

	[[nodiscard]] size_t size() const noexcept
	{
		return _arguments.size();
	}

protected:
	template <class _T, class... _Rest>
	[[nodiscard]] constexpr void _serialize(
		const _T& first,
		const _Rest&... rest)
	{
		static_assert(std::is_copy_constructible<_T>::value, "Cannot assign <_T> type, because isn't CopyConstructible!");
		_arguments.emplace_back(std::make_any<_T>(first));

		if constexpr (sizeof...(_Rest) > 0)
			_serialize(rest...);
	}

	template <class _T, class... _Rest>
	[[nodiscard]] constexpr std::tuple<_T, _Rest...> _deserialize(
		Parameters&& args) const
	{
		try
		{
			auto oValue = std::any_cast<_T>(args.front());
			auto oTuple = std::make_tuple(oValue);
			args.pop_front();

			if constexpr (sizeof...(_Rest) > 0)
				return std::tuple_cat(oTuple, _deserialize<_Rest...>(std::move(args)));

			return std::tuple_cat(oTuple, std::tuple<_Rest...>());
		}
		catch (const std::bad_any_cast& ex)
		{
			using namespace std::string_literals;
			throw std::invalid_argument("Wrong input type "s + ex.what() + "!");
		}
	}

protected:
	Parameters _arguments;
};

#else // C++14

class parameter_pack
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
	using Parameters				 = std::list<std::shared_ptr<parameter_base>>;
	parameter_pack()			 = default;
	virtual ~parameter_pack()	 = default;

	template <class... _Args>
	parameter_pack(
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
const _T& storage::parameter_pack::parameter_base::get() const
{
	using TRetrievedType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<parameter<_T>>().get())>::type>::type;
	static_assert(std::is_same<_T, TRetrievedType>::value, "Cannot cast templated return type <_T> to the derived class \"parameter.<_T>Get()\" type!");

	return dynamic_cast<const parameter<_T>&>(*this).get();
}

#endif // __cplusplus >= __cpp_lib_any



} // namespace janecekvit::storage
