#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
	StrikingDummy::Stats stats;
	stats.weapon_damage = 147;
	stats.main_stat = 3237;
	stats.critical_hit = 2681;
	stats.direct_hit = 1375;
	stats.determination = 994;
	stats.skill_speed = 1450;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//practice.start();
}