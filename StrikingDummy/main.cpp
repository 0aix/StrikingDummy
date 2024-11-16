#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

// Stats(float wd, float stat, float pot, float crit, float dh, float det, float sks)
StrikingDummy::Stats new_bis(146, 5126, 392, 3321, 1882, 1572, 1047);
StrikingDummy::Stats mid_bis(146, 5126, 392, 3321, 1558, 1356, 1587);
StrikingDummy::Stats fast_bis(146, 5126, 392, 1129, 2055, 2050, 2588);
StrikingDummy::Stats speed_bis(146, 5126, 392, 1129, 1623, 2050, 3020);

int main()
{
	StrikingDummy::BlackMage blm(new_bis, StrikingDummy::BlackMage::Opener::PRE_F3, StrikingDummy::BlackMage::ActionSet::FULL);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	
	// reminder: these other functions may not have been updated to log state correctly
	//dummy.metrics();
	//dummy.montecarlo();
	//dummy.dist(450, 10000);
	//dummy.study(0);
	//dummy.study(1);
	//dummy.study(2);
	//practice.start();
	//dummy.mp_offset();
}