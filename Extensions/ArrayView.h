#pragma once

#include <memory>
#include <string>

namespace Extensions
{
/// <summary>
/// Lightweight implementation of the std::span, until span will be fully supported in C++20 Visual Studio.
/// </summary>
template <class T>
class ArrayView
{
	using TPointer = T*;
public:
	constexpr ArrayView(TPointer pPtr, size_t iIndex)
		: m_pValue(pPtr)
		, m_iIndex(static_cast<std::ptrdiff_t>(iIndex))
	{

	}

	constexpr ArrayView(std::basic_string<T>&& data)
		: m_pValue(data.data())
		, m_iIndex(static_cast<std::ptrdiff_t>(data.size()))
	{

	}
	virtual ~ArrayView() = default;

	constexpr TPointer Data() const noexcept
	{
		return m_pValue;
	}
	constexpr std::ptrdiff_t Size() const noexcept
	{
		return m_iIndex;
	}

private:
	const TPointer m_pValue {};
	const std::ptrdiff_t m_iIndex {};
};

}