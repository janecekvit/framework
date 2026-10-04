#pragma once
#include "compatibility/compiler_support.h"
#include "extensions/constraints.h"

#include <functional>

namespace janecekvit::extensions
{

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
			_invoke_callback();
		}
		catch (const std::exception& ex)
		{
			if constexpr (!std::is_same_v<_E, constraints::default_exception_callback>)
				_exception_callback(ex);
		}
		catch (...)
		{
		}
	}

private:
	FRAMEWORK_NOINLINE void _invoke_callback() const
	{
		_callback();
	}

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
template <std::invocable _F, class _E>
	requires std::invocable<_E, const std::exception&>
final_action(_F&&, _E&&)
	-> final_action<_F, _E>;

/// <summary>
/// make_finally wraps final_action to easily call it
/// </summary>
template <std::invocable _F>
[[nodiscard]] decltype(auto) make_finally(_F&& callback)
{
	return final_action<_F>{ std::forward<_F>(callback) };
}

/// <summary>
/// make_finally wraps final_action to easily call it.
/// </summary>
template <std::invocable _F, class _E>
[[nodiscard]] decltype(auto) make_finally(_F&& callback, _E&& exception_callback)
{
	return final_action<_F, _E>{ std::forward<_F>(callback), std::forward<_E>(exception_callback) };
}

} // namespace janecekvit::extensions
