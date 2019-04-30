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

ICloneable.h
Purpose:	header file contains clone pattern mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 17/03/2019
*/

#include <memory>

/// <summary>
/// Interface implementing cloneable pattern with unique pointer, where templated type is type of the used class.
/// </summary>
/// <example>
/// <code>
///  class Implementation : public virtual ICloneable<Implementation>
///  {
///  public:
///		Implementation() {}
///  	virtual ~Implementation() {}
///  	virtual Implementation* CloneImpl() const override
///  	{
///  		return new Implementation(*this);
///  	}
///  };
/// </code>
/// </example>
template <class TDerived>
class ICloneable
{
public:
	virtual ~IClonePattern() = default;
	std::unique_ptr<TDerived> Clone() const
	{
		return std::make_unique<TDerived>(CloneImpl());
	}

	virtual ICloneable* CloneImpl() const = 0;
};
