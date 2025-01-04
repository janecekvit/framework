#pragma once
#include "extensions/constraints.h"

#include <functional>

namespace janecekvit::extensions
{

#ifdef __cpp_lib_concepts
/// <summary>
/// final_action implements finally semantics in C++.
/// Use it to release correctly resource when scope ends.
/// When custom exception callback is set, exceptions can be cached during object's destruction
/// </summary>
template <std::invocable _F, class _E = constraints::default_exception_callback>
class final_action
{
public:
	final_action(_F&& callback)
		: _callback(std::forward<_F>(callback))
	{
	}

	final_action(_F&& callback, _E&& exception_callback)
		requires(std::is_invocable_v<_E, const std::exception&>)
		: _callback(std::forward<_F>(callback))
		, _exception_callback(std::forward<_E>(exception_callback))
	{
	}

	virtual ~final_action()
	{
		try
		{
			_callback();
		}
		catch (const std::exception& ex)
		{
			if constexpr (!std::is_same_v<_E, constraints::default_exception_callback>)
				_exception_callback(ex);
		}
	}

private:
	const _F _callback{};
	const _E _exception_callback{};
};

///// <summary>
///// User defined deduction guide CTAD for final_action using default exception callback
///// </summary>
template <std::invocable _F>
final_action(_F&&)
	-> final_action<_F, constraints::default_exception_callback>;

///// <summary>
///// User-defined deduction guide CTAD for final_action using custom exception callback
///// </summary>
template <std::invocable _F, std::invocable _E>
final_action(_F&&, _E&&)
	-> final_action<_F, _E>;

/// <summary>
/// finally call wraps final_action to easily call it
/// </summary>
template <std::invocable _F, class... _Args>
[[nodiscard]] decltype(auto) finally(_F&& callback)
{
	return final_action<_F>{ std::forward<_F>(callback) };
}

/// <summary>
/// finally call wraps final_action to easily call it.
/// </summary>
template <std::invocable _F, class _E, class... _Args>
[[nodiscard]] decltype(auto) finally(_F&& callback, _E&& exception_callback)
{
	return final_action<_F, _E>{ std::forward<_F>(callback), std::forward<_E>(exception_callback) };
}

#else
/// <summary>
/// final_action implements finally semantics in C++.
/// Use it to release correctly resource when scope ends.
/// </summary>
template <class _F>
class final_action
{
public:
	template <std::enable_if_t<std::is_invocable_v<_F>, int> = 0>
	final_action(_F&& callback)
		: _callback(std::forward<_F>(callback))
	{
	}

	virtual ~final_action()
	{
		try
		{
			_callback();
		}
		catch (const std::exception&)
		{
		} // check double exception serious error
	}

private:
	const _F _callback{};
};

/// <summary>
/// finally call wraps final_action to easily call it
/// </summary>
template <class _F, class... _Args, std::enable_if_t<std::is_invocable_v<_F>, int> = 0>
[[nodiscard]] decltype(auto) finally(_F&& callback)
{
	return final_action<_F>{ std::forward<_F>(callback) };
}
#endif

} // namespace janecekvit::extensions
