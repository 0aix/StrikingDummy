#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

// Stats(float wd, float stat, float pot, float crit, float dh, float det, float sks)
StrikingDummy::Stats speed_bis(120, 2699, 189, 715, 1454, 1422, 2171);
StrikingDummy::Stats mid_bis(120, 2699, 189, 2323, 893, 1194, 1352);
StrikingDummy::Stats slow_bis(120, 2699, 189, 2323, 1037, 1518, 884);

int main()
{
	//StrikingDummy::BlackMage blm(speed_bis, StrikingDummy::BlackMage::Opener::PRE_T3, StrikingDummy::BlackMage::ActionSet::FULL);
	StrikingDummy::BlackMage blm(speed_bis, StrikingDummy::BlackMage::Opener::PRE_F3, StrikingDummy::BlackMage::ActionSet::FULL);
	//StrikingDummy::BlackMage blm(speed_bis, StrikingDummy::BlackMage::Opener::PRE_B3, StrikingDummy::BlackMage::ActionSet::STANDARD);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//dummy.dist(450, 10000);
	//dummy.study(0);
	//dummy.study(1);
	//dummy.study(2);
	//practice.start();
	//dummy.mp_offset();
}