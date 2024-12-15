#include "stdafx.h"

#include "CppUnitTest.h"
#include "extensions/cloneable.h"

using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;


struct impl : public virtual cloneable<impl>
{
	std::unique_ptr<impl> clone() const override
	{
		return std::make_unique<impl>(*this);
	}
};

namespace FrameworkTesting
{
ONLY_USED_AT_NAMESPACE_SCOPE class test_cloneable : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<test_cloneable> // expanded TEST_CLASS() macro due wrong formatting of clangformat
{
public:
	TEST_METHOD(TestCloneable)
	{
		auto s = std::make_unique<impl>();
		auto f = s->clone();
		Assert::IsNotNull(s.get());
		Assert::IsNotNull(f.get());
	};
};

} // namespace FrameworkTesting