#pragma once
#include <functional>

/// <summary>
/// CFinally class implements finally block in C++
/// </summary>
class CFinally
{
public:
	CFinally(std::function<void(void)>&& fnCallback)
		: m_fnCallback(std::move(fnCallback))
	{
	}

	virtual ~CFinally()
	{
		try
		{
			m_fnCallback();
		}
		catch (const std::exception& ) {} //check double exception serious error
		
	}

private:
	const std::function<void(void)> m_fnCallback {};
};