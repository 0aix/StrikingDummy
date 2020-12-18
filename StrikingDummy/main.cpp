#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

StrikingDummy::Stats max_speed(179, 5882, 464, 606, 3545, 1791, 3661);
StrikingDummy::Stats max_crit(180, 5884, 464, 3952, 2426, 1288, 1946);

int main()
{
	StrikingDummy::BlackMage blm(max_crit, StrikingDummy::BlackMage::Opener::PRE_LL_F3, StrikingDummy::BlackMage::ActionSet::FULL);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//dummy.dist(450, 10000);
	//dummy.study();
	//practice.start();
}