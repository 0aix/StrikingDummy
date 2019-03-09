#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Mimu.h"
#include "Samurai.h"
#include "Machinist.h"
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
#ifdef SAMURAI
	StrikingDummy::Stats stats;
	stats.weapon_damage = 109;
	stats.main_stat = 3182;
	stats.critical_hit = 2765;
	stats.direct_hit = 1919;
	stats.determination = 1144;
	stats.skill_speed = 892;
	stats.auto_attack = 95.92f;
	stats.auto_delay = 2.64f;

	StrikingDummy::Samurai sam(stats);
	StrikingDummy::TrainingDummy dummy(sam);
	StrikingDummy::StrikingDummy practice(sam);
	dummy.train();
	//dummy.trace_sam();
	//practice.start();
#endif
#ifdef MACHINIST
	StrikingDummy::Stats stats;
	stats.weapon_damage = 109;
	stats.main_stat = 3226;
	stats.critical_hit = 2656;
	stats.direct_hit = 1908;
	stats.determination = 1211;
	stats.skill_speed = 793;
	stats.auto_attack = 95.92f;
	stats.auto_delay = 2.64f;

	StrikingDummy::Machinist mch(stats);
	StrikingDummy::TrainingDummy dummy(mch);
	StrikingDummy::StrikingDummy practice(mch);
	dummy.train();
	//dummy.trace_mch();
	//practice.start();
#endif
}