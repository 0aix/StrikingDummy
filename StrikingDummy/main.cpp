#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Samurai.h"
#include "Logger.h"
#include <iostream>

int main()
{
	StrikingDummy::Stats stats;

	// samurai
	stats.weapon_damage = 128;
	stats.main_stat = 5101;
	stats.pot_stat = stats.main_stat + 398;
	stats.critical_hit = 3466;
	stats.direct_hit = 1429;
	stats.determination = 1030;
	stats.skill_speed = 3253;
	stats.auto_attack = 112.64;
	stats.auto_delay = 2.64;

	StrikingDummy::Samurai sam(stats);
	StrikingDummy::TrainingDummy dummy(sam);
	StrikingDummy::StrikingDummy practice(sam);
	dummy.train();
	//dummy.trace();
	//practice.start();
}