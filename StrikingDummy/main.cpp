#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

StrikingDummy::Stats max_speed(179, 5882, 464, 606, 3545, 1791, 3661);
StrikingDummy::Stats max_crit(180, 5884, 464, 3952, 2426, 1288, 1946);
StrikingDummy::Stats relic_speed(180, 5884, 464, 611, 3590, 1813, 3757);
StrikingDummy::Stats relic_crit(180, 5884, 464, 4142, 2065, 1635, 1929);

int main()
{
	//StrikingDummy::BlackMage blm(relic_crit, StrikingDummy::BlackMage::Opener::PRE_T3, StrikingDummy::BlackMage::ActionSet::FULL);
	StrikingDummy::BlackMage blm(relic_crit, StrikingDummy::BlackMage::Opener::PRE_F3, StrikingDummy::BlackMage::ActionSet::FULL);
	//StrikingDummy::BlackMage blm(relic_crit, StrikingDummy::BlackMage::Opener::PRE_B3, StrikingDummy::BlackMage::ActionSet::STANDARD);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	//dummy.train();
	dummy.trace();
	//dummy.metrics();
	//dummy.dist(450, 10000);
	//dummy.study(0);
	//dummy.study(1);
	//dummy.study(2);
	//practice.start();
	//dummy.mp_offset();
}