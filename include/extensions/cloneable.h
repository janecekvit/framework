#pragma once
/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2024 Vit Janecek. All rights reserved.

cloneable.h
Purpose:	header file contains clone pattern mechanism

@author:  Vit Janecek
@mailto:  <mailto:janecekvit@outlook.com>
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
