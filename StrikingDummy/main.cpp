#include "StrikingDummy.h"
#include "Logger.h"
#include <iostream>

/*
Notes:

- Caster tax is appended to a cast like an animation lock rather than prepended as lag. 

- Updates (MP, dots, timers, etc.) happen before actions are used. For example, if the TC duration hits 0 seconds, the TC proc 
  will be removed before evaluating actions. On the other hand, if the triple cast cooldown hits 0 seconds, the oGCD will be
  considered off cooldown. (Basically, it's not possible to use a proc at exactly 0 seconds left on its duration.)

- T3 dot and buffs apply immediately.

*/

int main()
{
	Logger::open();

	std::cout.precision(std::numeric_limits<double>::max_digits10);

	StrikingDummy::Stats stats;
	//stats.weapon_damage = 147;
	//stats.main_stat = 3237;
	//stats.critical_hit = 2681;
	//stats.direct_hit = 1375;
	//stats.determination = 994;
	//stats.skill_speed = 1450;
	stats.weapon_damage = 147;
	stats.main_stat = 3224;
	stats.critical_hit = 2461;
	stats.direct_hit = 1200;
	stats.determination = 1138;
	stats.skill_speed = 1504;

	StrikingDummy::BlackMage blm(stats);

	blm.start(new StrikingDummy::BalanceRotation(), 600);

	Logger::close();

	std::cin.get();
	return 0;
}