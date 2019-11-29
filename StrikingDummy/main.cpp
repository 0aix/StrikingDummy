#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Summoner.h"
#include "Logger.h"
#include <iostream>

int main()
{
	StrikingDummy::Stats stats;

	// 2.48s gcd
	stats.weapon_damage = 164;
	stats.main_stat = 4448; // 4237
	stats.pot_stat = stats.main_stat + 339;
	stats.critical_hit = 3516;
	stats.direct_hit = 2752;
	stats.determination = 1954;
	stats.skill_speed = 565;

	StrikingDummy::Summoner smn(stats);
	StrikingDummy::TrainingDummy dummy(smn);
	StrikingDummy::StrikingDummy practice(smn);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//practice.start();
	//dummy.dist(510, 30);
}