#include "Job.h"
#include "Rotation.h"
#include <assert.h>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

#define LV_SUB 364.0f
#define LV_DIV 2170.0f
#define LV_MAIN 292.0f
#define JOB_ATT 115.0f

namespace StrikingDummy
{
	// ============================================ Job ============================================

	Job::Job(Stats& stats) : stats(stats)
	{
		this->stats.calculate_stats();
		prob = std::uniform_real_distribution<float>(0.0f, 1.0f);
		damage_range = std::uniform_real_distribution<float>(0.95f, 1.05f);
		tick = std::uniform_int_distribution<int>(1, 300);
	}

	void Job::step()
	{
		int elapsed;
		while (!timeline.events.empty())
		{
			if ((elapsed = timeline.next_event()) > 0)
			{
				update(elapsed);
				// need at least 1 useable action that is not NONE (0)
				if (actions.empty() || (actions.size() == 1 && actions[0] == 0))
					continue;
				break;
			}
		}
	}

	void Job::seed(unsigned long long seed)
	{
		rng = std::mt19937(seed);
	}

	void Job::push_event(int offset)
	{
		if (offset > 0)
			timeline.push_event(offset);
	}

	// ============================================ Stats ============================================

	void Stats::calculate_stats()
	{
		wep_multiplier = floor(LV_MAIN * JOB_ATT / 1000.0f + weapon_damage);
		attk_multiplier = floor(125.0f * (main_stat - LV_MAIN) / LV_MAIN + 100.0f) / 100.0f;
		crit_multiplier = floor(200.0f * (critical_hit - LV_SUB) / LV_DIV + 1400.0f) / 1000.0f;
		crit_rate = floor(200.0f * (critical_hit - LV_SUB) / LV_DIV + 50.0f) / 1000.0f;
		dhit_rate = floor(550.0f * (direct_hit - LV_SUB) / LV_DIV) / 1000.0f;
		det_multiplier = floor(130.0f * (determination - LV_MAIN) / LV_DIV + 1000.0f) / 1000.0f;
		ss_multiplier = 1000.0f - floor(130.0f * (skill_speed - LV_SUB) / LV_DIV);
		dot_multiplier = floor(130.0f * (skill_speed - LV_SUB) / LV_DIV + 1000.0f) / 1000.0f;

		potency_multiplier = wep_multiplier * attk_multiplier * det_multiplier / 100.0f;
		float dcrit_rate = crit_rate * dhit_rate;
		expected_multiplier = (1.0f - crit_rate + dcrit_rate - dhit_rate) + crit_multiplier * (crit_rate - dcrit_rate) + crit_multiplier * 1.25f * dcrit_rate + 1.25f * (dhit_rate - dcrit_rate);
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