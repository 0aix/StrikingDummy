#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Summoner.h"
#include "Logger.h"
#include <iostream>

int main()
{
	StrikingDummy::Stats stats;

	// max speed
	stats.weapon_damage = 172;
	stats.main_stat = 5110; // 4867
	stats.pot_stat = stats.main_stat + 398;
	stats.critical_hit = 1026;
	stats.direct_hit = 2734;
	stats.determination = 1915;
	stats.skill_speed = 3503;

	StrikingDummy::Summoner smn(stats);
	StrikingDummy::TrainingDummy dummy(smn);
	StrikingDummy::StrikingDummy practice(smn);
	dummy.train();
	//dummy.trace();
	//dummy.metrics();
	//practice.start();
	//dummy.dist(510, 30);
}