#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
	StrikingDummy::Stats stats;

	// max speed set
	stats.weapon_damage = 172;
	stats.main_stat = 5110; // 4867
	stats.pot_stat = stats.main_stat + 398;
	stats.critical_hit = 528;
	stats.direct_hit = 2854;
	stats.determination = 1915;
	stats.skill_speed = 3881;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//practice.start();
	//dummy.dist(510, 10000);
}