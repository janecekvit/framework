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

IGetterSetter.h
Purpose:	header file contains Getter/Setter mechanism

@author: Vit Janecek
@mailto: <mailto:janecekvit@outlook.com>
@version 1.01 05/12/2019
*/

template <class TResource>
class IGetterSetter
{
public:
	IGetterSetter(TResource && oResource)
		: m_oResource(std::move(oResource))
	{
	}

	IGetterSetter(const TResource& oResource)
		: m_oResource(oResource)
	{
	}

	virtual ~IGetterSetter() = default;
	IGetterSetter(const IGetterSetter&) = default;
	IGetterSetter& operator=(const IGetterSetter&) = default;
	IGetterSetter(IGetterSetter&&) = default;
	IGetterSetter& operator=(IGetterSetter&&) = default;

	operator auto() const& -> const TResource&
	{
		return m_oResource;
	}

	operator auto() & -> TResource&
	{
		return m_oResource;
	}

	operator auto() && -> TResource&&
	{
		return std::move(m_oResource);
	}

protected:
	TResource m_oResource = {};
};