#pragma once
/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2019 Vit janecek <mailto:janecekvit@outlook.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

ICRTP.h
Purpose:	header file contains CRTP pattern mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 17/03/2019
*/

#include <iostream>
#include <memory>

/// <summary>
/// Interface implementing Curiously recurring template pattern.
/// CRTP can perform lazy evaluation on underlay method by static cast.
/// </summary>
/// <example>
/// <code>
// template <class T>
// struct Base : ICRTP<T>
// {
//	void interface()
//	{
//		// ...
//		UnderlyingType().implementation();
//		// ...
//	}
//
//	void implementation()
//	{
//		// ...
//	}
// };
//
// struct Derived : Base<Derived>
// {
//	void implementation()
//	{
//		// ...
//	}
// };
//
// struct Derived2 : Base<Derived2>
// {
// };
/// </code>
/// </example>
template <typename TDerived>
class ICRTP
{
public:
	virtual ~ICRTP() = default;

	TDerived& UnderlyingType()
	{
		return static_cast<TDerived&>(*this);
	}

	const TDerived& UnderlyingType() const
	{
		return static_cast<const TDerived&>(*this);
	}
};
