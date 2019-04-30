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
