#include "StrikingDummy.h"

namespace StrikingDummy
{
	void BalanceRotation::start(BlackMage* blm)
	{
		this->timeline = new Timeline();
		this->blm = blm;

		// Precast event

		//
		for (Event* e = timeline->next_event(); e != NULL; delete e)
		{

		}
	}
}