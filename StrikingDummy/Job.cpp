#include "Job.h"
#include "Rotation.h"
#include <assert.h>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

#define LV_SUB 364.0
#define LV_DIV 2170.0
#define LV_MAIN 292.0
#define JOB_ATT 115.0

namespace StrikingDummy
{
	// ============================================ Job ============================================

	Job::Job(Stats& stats) : stats(stats)
	{
		this->stats.calculate_stats();
	}

	void Job::step()
	{
		int elapsed;
		while (!timeline.events.empty())
		{
			if ((elapsed = timeline.next_event()) > 0)
			{
				update(elapsed);
				break;
			}
		}
	}

	void Job::push_event(int offset)
	{
		if (offset > 0)
			timeline.push_event(offset);
	}

	// ============================================ Stats ============================================

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

		potency_multiplier = wep_multiplier * attk_multiplier * det_multiplier / 100.0;
		double dcrit_rate = crit_rate * dhit_rate;
		expected_multiplier = (1 - crit_rate + dcrit_rate - dhit_rate) + crit_multiplier * (crit_rate - dcrit_rate) + crit_multiplier * 1.25 * dcrit_rate + 1.25 * (dhit_rate - dcrit_rate);
	}

	// ============================================ Timeline ============================================

	int Timeline::next_event()
	{
		if (events.empty())
			return 0;
		int temp = events.top();
		int elapsed = temp - time;
		time = temp;
		events.pop();
		return elapsed;
	}

	void Timeline::push_event(int offset)
	{
		events.push(time + offset);
	}

	// ============================================ Timer ============================================

	void Timer::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			ready = true;
		DBG(assert(time >= 0));
	}

	void Timer::reset(int duration, bool ready)
	{
		this->time = duration;
		this->ready = ready;
	}

	// ============================================ Buff ============================================

	void Buff::update(int elapsed)
	{
		if (time > 0 && (time -= elapsed) == 0)
			count = 0;
		DBG(assert(time >= 0));
	}

	void Buff::reset(int duration, int count)
	{
		this->time = duration;
		this->count = count;
	}
}