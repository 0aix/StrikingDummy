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
	//stats.weapon_damage = 164;
	//stats.main_stat = 4448; // 4237
	//stats.pot_stat = stats.main_stat + 339;
	//stats.critical_hit = 578;
	//stats.direct_hit = 3129;
	//stats.determination = 2021;
	//stats.skill_speed = 3057;

	// max crit set
	stats.weapon_damage = 164;
	stats.main_stat = 4448; // 4237
	stats.pot_stat = stats.main_stat + 339;
	stats.critical_hit = 3387;
	stats.direct_hit = 2056;
	stats.determination = 1103;
	stats.skill_speed = 2241;

	// max sps set
	//stats.weapon_damage = 164;
	//stats.main_stat = 4448; // 4237
	//stats.pot_stat = stats.main_stat + 339;
	//stats.critical_hit = 1040;
	//stats.direct_hit = 2689;
	//stats.determination = 1559;
	//stats.skill_speed = 3500;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//practice.start();
	//dummy.dist(510, 30);
}