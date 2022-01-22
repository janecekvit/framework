#pragma once
#include "extensions/constraints.h"

#include <functional>

namespace janecekvit::extensions
{
/// <summary>
/// final_action holder implements finally semantics in C++
/// </summary>
template <class _F>
class final_action
{
public:
	template <std::enable_if_t<std::is_invocable_v<_F>, int> = 0>
	final_action(_F&& fnCallback)
		: _callback(std::forward<_F>(fnCallback))
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
		} //check double exception serious error
	}

private:
	const _F _callback{};
};

/// <summary>
/// finally call wraps final_action to easily retrieve and fill it
/// </summary>
template <class _F, class... _Args, std::enable_if_t<std::is_invocable_v<_F>, int> = 0>
[[nodiscard]] decltype(auto) finally(_F&& callback)
{
	return final_action<_F>{ std::forward<_F>(callback) };
}

} // namespace janecekvit::extensions
