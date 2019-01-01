#pragma once

#include <queue>

namespace StrikingDummy
{
	struct Stats
	{
		int weapon_damage;
		int main_stat;
		int critical_hit;
		int direct_hit;
		int determination;
		int skill_speed;

		float wep_multiplier;
		float attk_multiplier;
		float crit_rate;
		float crit_multiplier;
		float dhit_rate;
		float det_multiplier;
		float ss_multiplier;
		float dot_multiplier;
		
		float potency_multiplier;
		float expected_multiplier;

		void calculate_stats();
	};

	struct Timeline
	{
		std::priority_queue<int, std::vector<int>, std::greater<int>> events;
		int time = 0;

		int next_event();
		void push_event(int offset);
	};

	struct Timer
	{
		int time = 0;
		bool ready = false;

		void update(int elapsed);
		void reset(int duration, bool ready);
	};

	struct Buff
	{
		int time = 0;
		int count = 0;

		void update(int elapsed);
		void reset(int duration, int count);
	};

	struct Job
	{
		Stats stats;
		Timeline timeline;
		std::vector<int> actions;
		long long total_damage = 0;

		Job(Stats& stats);

		void step();

		virtual void reset() = 0;
		virtual void use_action(int action) = 0;
		virtual void* get_history() = 0;
		virtual void* get_state() = 0;

	protected:
		virtual void update(int elapsed) = 0;

		void push_event(int offset);
	};
}