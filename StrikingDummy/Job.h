#pragma once

#include <queue>
#include <random>

namespace StrikingDummy
{
	struct Stats
	{
		float flat_atk;
		float base_atk_per;
		float refined_atk;
		float str;
		float base_str_per;
		float crit;
		float haste;
		float luck;
		float mastery;
		float vers;
		float base_atk_spd;
		float base_crit_multi;
		float base_luck_multi;
		float base_armor_pen;
		float base_ele_stat;
		float base_serum_stat;
		float base_dmg;

		Stats() {}
		Stats(float flat_atk, float base_atk_per, float refined_atk, float str, float base_str_per, float crit, float haste, float luck, float mastery, float vers, float base_atk_spd, float base_crit_multi, float base_luck_multi, float base_armor_pen, float base_ele_stat, float base_serum_stat, float base_dmg) :
			flat_atk(flat_atk), base_atk_per(base_atk_per), refined_atk(refined_atk), str(str), base_str_per(base_str_per), crit(crit), haste(haste), luck(luck), mastery(mastery), vers(vers), base_atk_spd(base_atk_spd), base_crit_multi(base_crit_multi), base_luck_multi(base_luck_multi), base_armor_pen(base_armor_pen), base_ele_stat(base_ele_stat), base_serum_stat(base_serum_stat), base_dmg(base_dmg) {}

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
		int time = 0;
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

		double total_damage = 0.0f;
		double pre_damage = 0.0f;

		Job(Stats& job_stats);
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