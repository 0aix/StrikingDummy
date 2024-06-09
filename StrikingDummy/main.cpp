#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

// Stats(float wd, float stat, float pot, float crit, float dh, float det, float sks)
StrikingDummy::Stats new_bis(132, 3378, 262, 2514, 1402, 1601, 716);
StrikingDummy::Stats speed_bis(132, 3378, 262, 851, 1439, 1422, 2521);

int main()
{
	StrikingDummy::BlackMage blm(speed_bis, StrikingDummy::BlackMage::Opener::PRE_F3, StrikingDummy::BlackMage::ActionSet::FULL);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	
	// reminder: these other functions have not been updated to log state correctly
	//dummy.metrics();
	//dummy.montecarlo();
	//dummy.dist(450, 10000);
	//dummy.study(0);
	//dummy.study(1);
	//dummy.study(2);
	//practice.start();
	//dummy.mp_offset();
}