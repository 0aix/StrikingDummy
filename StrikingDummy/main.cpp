#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
	StrikingDummy::Stats stats;
	stats.weapon_damage = 158;
	stats.main_stat = 3726;
	stats.pot_stat = 3726 + 312;
	stats.critical_hit = 1692;
	stats.direct_hit = 2121;
	stats.determination = 1510;
	stats.skill_speed = 2388;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//practice.start();
}