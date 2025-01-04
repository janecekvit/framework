#include "conversions.h"

#include <codecvt>
#include <locale>
#include <memory>

namespace janecekvit::conversions
{
std::string to_string(std::wstring_view&& view)
{
#pragma warning(disable : 4996)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(view.data());
#pragma warning(default : 4996)
}

std::wstring to_wstring(std::string_view&& view)
{
#pragma warning(disable : 4996)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(view.data());
#pragma warning(default : 4996)
}

} // namespace janecekvit::conversions
