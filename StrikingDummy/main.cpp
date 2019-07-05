#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Monk.h"
#include "Logger.h"
#include <iostream>

int main()
{
	StrikingDummy::Stats stats;
	stats.weapon_damage = 109;
	stats.main_stat = 3193;
	stats.pot_stat = 3193 + 225;
	stats.critical_hit = 2484;
	stats.direct_hit = 1994;
	stats.determination = 1330;
	stats.skill_speed = 819;
	stats.auto_attack = 93.01f;
	stats.auto_delay = 2.56f;

	StrikingDummy::Monk monk(stats);
	StrikingDummy::TrainingDummy dummy(monk);
	StrikingDummy::StrikingDummy practice(monk);
	dummy.train();
	//dummy.trace();
	//practice.start();
}