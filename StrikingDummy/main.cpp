#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Monk.h"
#include "Logger.h"
#include <iostream>

StrikingDummy::Stats stats(128, 5095, 398, 3996, 2116, 2041, 1025, 109.23f, 2.56f);

int main()
{
	StrikingDummy::Monk monk(stats);
	StrikingDummy::TrainingDummy dummy(monk);
	StrikingDummy::StrikingDummy practice(monk);
	dummy.train();
	//dummy.trace();
	//practice.start();
}