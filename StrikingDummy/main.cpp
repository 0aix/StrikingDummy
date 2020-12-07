#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

StrikingDummy::Stats max_speed(172, 5110, 398, 528, 2854, 1915, 3881);
StrikingDummy::Stats max_crit(172, 5110, 398, 3570, 2477, 1192, 1939);

int main()
{
	StrikingDummy::BlackMage blm(max_crit, StrikingDummy::BlackMage::Opener::PRE_B3, StrikingDummy::BlackMage::ActionSet::NO_B4);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	//dummy.train();
	//dummy.trace();
	//dummy.metrics();
	dummy.dist(450, 10000);
	//dummy.study();
	//practice.start();
}