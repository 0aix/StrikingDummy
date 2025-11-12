#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

// Stats(float flat_atk, float refined_atk, float str, float crit, float haste, float luck, float mastery, float vers, float base_atk_spd, float base_crit_multi)
StrikingDummy::Stats stats(146, 5126, 392, 3321, 1882, 1572, 1047);

int main()
{
	StrikingDummy::BlackMage blm(stats, StrikingDummy::BlackMage::Opener::GAUGE);
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