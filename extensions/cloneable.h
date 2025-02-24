#pragma once
/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2024 Vit janecek <mailto:janecekvit@outlook.com>.

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

ICloneable.h
Purpose:	header file contains clone pattern mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
*/

#include <memory>

namespace janecekvit::extensions
{

/// <summary>
/// Interface for implementing the clone pattern.
/// </summary>
/// <typeparam name="_T">The type of the class implementing the interface.</typeparam>
/// <example>
/// <code>
/// struct impl : public virtual cloneable<impl>
/// {
/// 	std::unique_ptr<impl> clone() const override
/// 	{
///         return std::make_unique<impl>(*this);
/// 	}
/// };
/// </code>
/// </example>
template <class _T>
class cloneable
{
public:
	virtual ~cloneable() = default;

	/// <summary>
	/// Method to clone the current instance of the object and wrap it in a RAII memory wrapper.
	/// </summary>
	/// <returns>Returns a memory-safe free pointer to this instance of the object.</returns>
	virtual std::unique_ptr<_T> clone() const = 0;
};

} // namespace janecekvit::extensions
