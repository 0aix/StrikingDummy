#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Samurai.h"
#include "Logger.h"
#include <iostream>

int main()
{
	StrikingDummy::Stats stats;

	// samurai
	stats.weapon_damage = 122;
	stats.main_stat = 4441; // 4230
	stats.pot_stat = stats.main_stat + 312;
	stats.critical_hit = 3124;
	stats.direct_hit = 2938;
	stats.determination = 1893;
	stats.skill_speed = 814;
	stats.auto_attack = 107.36;
	stats.auto_delay = 2.64;

	StrikingDummy::Samurai sam(stats);
	StrikingDummy::TrainingDummy dummy(sam);
	StrikingDummy::StrikingDummy practice(sam);
	//dummy.train();
	dummy.trace();
	//practice.start();
}