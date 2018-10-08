#include "StrikingDummy.h"

namespace StrikingDummy
{
	Event* Timeline::next_event()
	{
		Event* e = NULL;
		if (!events.empty())
		{
			e = events.top();
			events.pop();
		}
		return e;
	}
}