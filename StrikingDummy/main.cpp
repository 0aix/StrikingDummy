#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
	StrikingDummy::Stats stats;

	// min crit set
	stats.weapon_damage = 164;
	stats.main_stat = 4448; // 4237
	stats.pot_stat = stats.main_stat + 312;
	stats.critical_hit = 578;
	stats.direct_hit = 3129;
	stats.determination = 2021;
	stats.skill_speed = 3057;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//practice.start();
}