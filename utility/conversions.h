#pragma once
#include <string>

namespace janecekvit::conversions
{
std::string to_string(std::wstring_view&& view);
std::wstring to_wstring(std::string_view&& view);

} // namespace janecekvit::conversions