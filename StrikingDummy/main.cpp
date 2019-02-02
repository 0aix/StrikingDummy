#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Mimu.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
#ifdef BLACKMAGE
	StrikingDummy::Stats stats;
	stats.weapon_damage = 147;
	stats.main_stat = 3237;
	stats.critical_hit = 2681;
	stats.direct_hit = 1375;
	stats.determination = 994;
	stats.skill_speed = 1450;
	//stats.weapon_damage = 147;
	//stats.main_stat = 3237;
	//stats.critical_hit = 2681;
	//stats.direct_hit = 1824;
	//stats.determination = 954;
	//stats.skill_speed = 1050;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	//dummy.train();
	dummy.trace();
	//practice.start();
#endif
#ifdef MONK
	StrikingDummy::Stats stats;
	stats.weapon_damage = 109;
	stats.main_stat = 3177;
	stats.critical_hit = 2571;
	stats.direct_hit = 1845;
	stats.determination = 1545;
	stats.skill_speed = 758;
	stats.auto_attack = 93.01f;
	stats.auto_delay = 2.56f;

	StrikingDummy::Mimu mimu(stats);
	StrikingDummy::TrainingDummy dummy(mimu);
	StrikingDummy::StrikingDummy practice(mimu);
	//dummy.train();
	dummy.trace_mimu();
	//practice.start();
#endif
}