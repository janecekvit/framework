#pragma once

#include <memory>

namespace Extensions
{

template <class T>
class ArrayView
{
	using TPointer = T*;
public:
	constexpr ArrayView(TPointer&& pPtr, ptrdiff_t iIndex)
		: m_pValue(pPtr)
		, m_iIndex(iIndex)
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