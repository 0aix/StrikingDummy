#include "StrikingDummy.h"

int main()
{
	StrikingDummy::BlackMageRotation* rotation = new StrikingDummy::BalanceRotation();

	StrikingDummy::BlackMage* blm = new StrikingDummy::BlackMage();

	blm->calculate_stats();

	rotation->start(blm);
}