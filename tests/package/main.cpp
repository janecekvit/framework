#include "extensions/finally.h"
#include "synchronization/concurrent.h"

int main()
{
	janecekvit::synchronization::concurrent::vector<int> shared_numbers;
	{
		auto cleanup = janecekvit::extensions::make_finally([&]
			{
				shared_numbers.exclusive()->push_back(1);
			});
	}

	return shared_numbers.concurrent()->size() == 1 ? 0 : 1;
}
