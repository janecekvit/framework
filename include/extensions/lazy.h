#pragma once

#include "extensions/constraints.h"

namespace janecekvit::extensions
{
/// <summary>
/// lazy action implements lazy evaluation of stored callable function
/// </summary>

template <class _Ret, class... _Args>
class lazy_action
{
public:
	template <class _F, std::enable_if_t<std::is_invocable_v<_F, _Args...>, int> = 0>
	lazy_action(_F&& evulator, _Args&&... args)
		: _evaluator(std::forward<_F>(evulator))
		, _arguments(std::make_tuple(std::forward<_Args>(args)...))
	{
	}

#ifdef __cpp_lib_concepts
	template <class... _AltArgs>
		requires std::is_same_v<std::tuple<_AltArgs...>, std::tuple<_Args...>> && std::is_invocable_v<std::function<_Ret(_Args...)>, _AltArgs...>
#else
	template <class... _AltArgs, std::enable_if_t<std::is_same_v<std::tuple<_AltArgs...>, std::tuple<_Args...>>, int> = 0>
#endif
	[[nodiscard]] _Ret operator()(_AltArgs&&... args) const // -> std::invoke_result_t<_F, _Args...>
	{
		return std::invoke(_evaluator, std::forward<_AltArgs>(args)...);
	}

	[[nodiscard]] _Ret operator()() const // -> std::invoke_result_t<_F, _Args...>
	{
		return std::apply(_evaluator, _arguments);
	}

private:
	const std::function<_Ret(_Args...)> _evaluator{};
	std::tuple<_Args...> _arguments;
};

/// <summary>
/// lazy call wraps final_action to easily retrieve and fill it
/// </summary>
#ifdef __cpp_lib_concepts
template <class _F, class... _Args>
	requires std::is_invocable_v<_F, _Args...>
#else
template <class _F, class... _Args, std::enable_if_t<std::is_invocable_v<_F, _Args...>, int> = 0>
#endif
[[nodiscard]] decltype(auto) lazy(_F&& callback, _Args&&... args)
{
	return lazy_action<std::invoke_result_t<_F, _Args...>, _Args...>{ std::forward<_F>(callback), std::forward<_Args>(args)... };
}

} // namespace janecekvit::extensions
