#include "stdafx.h"

#include "CppUnitTest.h"
#include "Framework/Extensions/ResourceWrapper.h"
#include "Framework/Interface/ICloneable.h"

#include <fstream>
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class Implementation : public virtual IClone<Implementation>
{
public:
	Implementation()		  = default;
	virtual ~Implementation() = default;

protected:
	Implementation* _CloneImpl() const override
	{
		return new Implementation(*this);
	}
};

class Implementation2 : public virtual ICloneable<Implementation2>
{
public:
	Implementation2()		   = default;
	virtual ~Implementation2() = default;

	std::unique_ptr<Implementation2> Clone() const
	{
		return std::unique_ptr<Implementation2>(_CloneImpl());
	}

protected:
	/// <summary>
	/// Implementation of RAII clone pattern mechanism.
	/// </summary>
	/// <returns>returns RAW pointer to this instance of object</returns>
	Implementation2* _CloneImpl() const
	{
		return new Implementation2(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
class Base : public ICRTP<T>
{
public:
	void interface()
	{
		//
		ICRTP<T>::UnderlyingType().implementation();
		// ...
	}

	void implementation()
	{
		i++;
	}

	int i = 0;
};

class Derived : public Base<Derived>
{
public:
	void implementation()
	{
		j++;
	}
	int j = 0;
};

class Derived2 : public Base<Derived2>
{
public:
	int j = 0;
};

///////////////////////////////////////////////////////////////////////////////
namespace FrameworkUT
{
TEST_CLASS(TestInterfaces)
{
public:
	TEST_METHOD(TestIcloneable)
	{
		auto s = std::make_unique<Implementation>();
		auto f = s->Clone();
		Assert::IsNotNull(s.get());
		Assert::IsNotNull(f.get());

		auto s2 = std::make_unique<Implementation2>();
		auto f2 = s2->Clone();

		Assert::IsNotNull(s2.get());
		Assert::IsNotNull(f2.get());

		//interface found, called derived class method
		Derived d1;
		d1.interface();
		Assert::AreEqual(d1.i, 0);
		Assert::AreEqual(d1.j, 1);

		//interface not found, called base class method
		Derived2 d2;
		d2.interface();
		Assert::AreEqual(d2.i, 1);
		Assert::AreEqual(d2.j, 0);
	};
};
} // namespace FrameworkUT