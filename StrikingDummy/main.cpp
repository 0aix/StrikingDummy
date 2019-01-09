#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>

int main()
{
	//std::cout << "state size: " << sizeof(StrikingDummy::State) << std::endl;
	//std::cout << "transition size: " << sizeof(StrikingDummy::Transition) << std::endl;

	std::cout.precision(std::numeric_limits<double>::max_digits10);

	StrikingDummy::Stats stats;
	stats.weapon_damage = 147;
	stats.main_stat = 3237;
	stats.critical_hit = 2681;
	stats.direct_hit = 1375;
	stats.determination = 994;
	stats.skill_speed = 1450;
	//stats.weapon_damage = 147;
	//stats.main_stat = 3237;
	//stats.critical_hit = 2606;
	//stats.direct_hit = 2013;
	//stats.determination = 954;
	//stats.skill_speed = 860;

	StrikingDummy::BlackMage blm(stats);
	StrikingDummy::TrainingDummy dummy(blm);
	//StrikingDummy::StrikingDummy practice(blm);

	dummy.train();
	//practice.start();

	std::cin.get();
	return 0;
}