#include "stdafx.h"

#include "extensions/cloneable.h"

#include <gtest/gtest.h>

using namespace std::string_literals;

struct impl : public virtual cloneable<impl>
{
	std::unique_ptr<impl> clone() const override
	{
		return std::make_unique<impl>(*this);
	}
};

namespace FrameworkTesting
{
class test_cloneable : public ::testing::Test
{
protected:
	void SetUp() override
	{
	}

	void TearDown() override
	{
	}
};

TEST_F(test_cloneable, TestCloneable)
{
	auto s = std::make_unique<impl>();
	auto f = s->clone();
	ASSERT_NE(nullptr, s.get());
	ASSERT_NE(nullptr, f.get());
};

} // namespace FrameworkTesting