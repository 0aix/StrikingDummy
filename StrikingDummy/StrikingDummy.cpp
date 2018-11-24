#include "StrikingDummy.h"
#include <assert.h>

#define LV_SUB 364.0
#define LV_DIV 2170.0
#define LV_MAIN 292.0
#define JOB_ATT 115.0

namespace StrikingDummy
{
	int Timeline::next_event()
	{
		int temp;
		int elapsed;
		if (!events.empty())
		{
			temp = events.top();
			elapsed = temp - time;
			time = temp;
			events.pop();
			return elapsed;
		}
		return -99999;
	}

	void Timeline::push_event(int offset)
	{
		events.push(time + offset);
	}

	void Buff::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			count = 0;
		DBG(assert(time >= 0));
	}

	void Buff::step()
	{
		if (time > 0 && --time == 0)
			count = 0;
	}

	void Buff::reset(int duration, int count)
	{
		this->time = duration;
		this->count = count;
	}

	void Timer::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			ready = true;
		DBG(assert(time >= 0));
	}

	void Timer::step()
	{
		if (time > 0 && --time == 0)
			ready = true;
	}

	void Timer::reset(int duration, bool ready)
	{
		this->time = duration;
		this->ready = ready;
	}

	void Stats::calculate_stats()
	{
		wep_multiplier = floor(LV_MAIN * JOB_ATT / 1000.0 + weapon_damage);
		attk_multiplier = floor(125.0 * (main_stat - LV_MAIN) / LV_MAIN + 100.0) / 100.0;
		crit_multiplier = floor(200.0 * (critical_hit - LV_SUB) / LV_DIV + 1400.0) / 1000.0;
		crit_rate = floor(200.0 * (critical_hit - LV_SUB) / LV_DIV + 50.0) / 1000.0;
		dhit_rate = floor(550.0 * (direct_hit - LV_SUB) / LV_DIV) / 1000.0;
		det_multiplier = floor(130.0 * (determination - LV_MAIN) / LV_DIV + 1000.0) / 1000.0;
		ss_multiplier = 1000.0 - floor(130.0 * (skill_speed - LV_SUB) / LV_DIV);
		dot_multiplier = floor(130.0 * (skill_speed - LV_SUB) / LV_DIV + 1000.0) / 1000.0;
	}
}