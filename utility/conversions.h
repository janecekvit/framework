#pragma once
#include "extensions/constraints.h"

#include <numeric>
#include <string>

namespace janecekvit::conversions
{
std::string to_string(std::wstring_view&& view);
std::wstring to_wstring(std::string_view&& view);

template <constraints::string_type _String, class _Container>
requires constraints::is_container_v<_Container>
	_String to_string(const _Container& container)
{
	return std::accumulate(container.begin(), container.end(), _String(), [](const auto& item1, const auto& item2)
		{
			return item1 + _String(", ") + item2;
		});
}

template <constraints::string_type _String, class _Container>
requires constraints::is_concurrent_container_v<_Container>
	_String to_string(const _Container& container)
{
	return to_string(container.concurrent().get());
}

} // namespace janecekvit::conversions