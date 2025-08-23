#pragma once
#include "extensions/constraints.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <numeric>

namespace janecekvit::conversions
{
inline std::string to_string(const std::wstring& str)
{
    if (str.empty())
        return {};

#ifdef _WIN32
    std::mbstate_t state = std::mbstate_t();
	const wchar_t* data = str.data();
	size_t len = 0;
	wcsrtombs_s(&len, nullptr, 0, &data, 0, &state);

	std::vector<char> buffer(len);

	state = std::mbstate_t();
	data = str.data();
	wcsrtombs_s(&len, buffer.data(), buffer.size(), &data, buffer.size() - 1, &state);
	return std::string(buffer.data());
#else
    std::mbstate_t state = std::mbstate_t();
    const wchar_t* data = str.data();
    size_t len = std::wcsrtombs(nullptr, &data, 0, &state);
    
    if (len == static_cast<size_t>(-1))
        return {}; // conversion error

    std::vector<char> buffer(len + 1);
    
    state = std::mbstate_t();
    data = str.data();
    std::wcsrtombs(buffer.data(), &data, len, &state);
    
    return std::string(buffer.data(), len);
#endif
}

inline std::wstring to_wstring(const std::string& str)
{
    if (str.empty())
        return {};

#ifdef _WIN32
    std::mbstate_t state = std::mbstate_t();
	const char* data = str.data();
	size_t len = 0;
	mbsrtowcs_s(&len, nullptr, 0, &data, 0, &state);

	std::vector<wchar_t> buffer(len);

	state = std::mbstate_t();
	data = str.data();
	mbsrtowcs_s(&len, buffer.data(), buffer.size(), &data, buffer.size() - 1, &state);
	return std::wstring(buffer.data());
#else

    std::mbstate_t state = std::mbstate_t();
    const char* data = str.data();
    size_t len = std::mbsrtowcs(nullptr, &data, 0, &state);
    
    if (len == static_cast<size_t>(-1))
        return {}; // conversion error

    std::vector<wchar_t> buffer(len + 1);
    
    state = std::mbstate_t();
    data = str.data();
    std::mbsrtowcs(buffer.data(), &data, len, &state);
    
    return std::wstring(buffer.data(), len);

#endif
}

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
