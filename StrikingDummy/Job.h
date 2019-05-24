#pragma once

#include <queue>
#include <random>

namespace StrikingDummy
{
	struct Stats
	{
		float weapon_damage;
		float main_stat;
		float critical_hit;
		float direct_hit;
		float determination;
		float skill_speed;
		float auto_attack;
		float auto_delay;

		float wep_multiplier;
		float attk_multiplier;
		float crit_rate;
		float crit_multiplier;
		float dhit_rate;
		float det_multiplier;
		float ss_multiplier;
		float dot_multiplier;
		float aa_multiplier;
		
		float potency_multiplier;
		float expected_multiplier;

		void calculate_stats(float job_attr);
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

	struct Transition
	{
		float t0[64];
		float t1[64];
		int action = 0;
		float reward = 0.0f;
		int dt = 0;
		std::vector<int> actions;
	};

	struct Job
	{
		Stats stats;
		Timeline timeline;
		std::vector<int> actions;
		std::vector<Transition> history;

		std::mt19937 rng;
		std::uniform_real_distribution<float> prob;
		std::uniform_real_distribution<float> damage_range;
		std::uniform_int_distribution<int> tick;

		float total_damage = 0.0f;

		Job(Stats& job_stats, float job_attr);
		void step();
		float* get_state() { return history.back().t0; }

		virtual void reset() = 0;
		virtual void use_action(int action) = 0;
		virtual void get_state(float* state) = 0;
		virtual int get_state_size() = 0;
		virtual int get_num_actions() = 0;
		virtual std::string get_action_name(int action) = 0;
		virtual std::string get_info() = 0;

	protected:
		virtual void update(int elapsed) = 0;

		void push_event(int offset);
	};
}