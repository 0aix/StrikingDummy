#pragma once

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

#include <queue>
#include <vector>

namespace StrikingDummy
{
	struct Job;

	struct Timeline
	{
		int time = 0;

		int next_event();
		void push_event(int offset);

	private:
		std::priority_queue<int, std::vector<int>, std::greater<int>> events;
	};

	struct Rotation
	{
		Job* job;

		virtual int choose_action(std::vector<int>& actions) = 0;
	};

	struct BalanceRotation : public Rotation
	{
		BalanceRotation();

		int choose_action(std::vector<int>& actions);

		int action_none();
		int action_b1();
		int action_b3();
		int action_b4();
		int action_f1();
		int action_f3();
		int action_f4();
		int action_t3();
		int action_foul();
		int action_swift();
		int action_triple();
		int action_sharp();
		int action_ll();
		int action_convert();
		int action_eno();
	};

	struct Stats
	{
		int weapon_damage;
		int main_stat;
		int critical_hit;
		int direct_hit;
		int determination;
		int skill_speed;

		double wep_multiplier;
		double attk_multiplier;
		double crit_rate;
		double crit_multiplier;
		double dhit_rate;
		double det_multiplier;
		double ss_multiplier;
		double dot_multiplier;

		void calculate_stats();
	};

	struct Job
	{
		Job(Stats stats) : stats(stats)
		{
			this->stats.calculate_stats();
		}

		virtual void start(Rotation* rotation, int time_limit_in_seconds) = 0;
		virtual void train() = 0;
		
	protected:
		virtual void update(int elapsed) = 0;
		virtual void step() = 0;

		void push_event(int offset)
		{
			if (!training && offset > 0)
				timeline.push_event(offset);	
		}

		Stats stats;
		Timeline timeline;
		std::vector<int> actions;
		bool training = false;
	};

	struct Timer
	{
		int time = 0;
		bool ready = false;

		void update(int elapsed);
		void step();
		void reset(int duration, bool ready);
	};

	struct Buff
	{
		int time = 0;
		int count = 0;

		void update(int elapsed);
		void step();
		void reset(int duration, int count);
	};

	struct BlackMage : public Job
	{
		enum Action
		{
			NONE,
			B1, B3, B4, F1, F3, F4, T3, FOUL, // Might want to code in F3P and T3P...
			SWIFT, TRIPLE, SHARP, LEYLINES, CONVERT, ENOCHIAN
		};

		enum Element
		{
			NE, UI, AF
		};

		static constexpr int NUM_ACTIONS = 15;

		static constexpr int CASTER_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		// Assume only in UI3 and AF3
		static constexpr int MAX_MP = 15480;
		static constexpr int MP_PER_TICK = 309;			// 2% per tick
		static constexpr int MP_PER_TICK_UI1 = 4953;	// 32% per tick in UI1
		static constexpr int MP_PER_TICK_UI2 = 7275;	// 47% per tick in UI2
		static constexpr int MP_PER_TICK_UI3 = 9597;	// 62% per tick in UI3
		static constexpr int CONVERT_MP = 4644;
		
		static constexpr double BASE_GCD = 2.50;
		static constexpr double III_GCD = 3.50;
		static constexpr double IV_GCD = 2.80;
		static constexpr double FAST_BASE_GCD = 1.25;
		static constexpr double FAST_III_GCD = 1.75;

		static constexpr int TICK_TIMER = 300;
		static constexpr int FOUL_TIMER = 3000;
		static constexpr int GAUGE_DURATION = 1300;
		static constexpr int SWIFT_DURATION = 1000;
		static constexpr int TRIPLE_DURATION = 1500;
		static constexpr int SHARP_DURATION = 1500;
		static constexpr int FS_DURATION = 1800;
		static constexpr int TC_DURATION = 1800;
		static constexpr int LL_DURATION = 3000;
		static constexpr int DOT_DURATION = 2400;

		static constexpr int SWIFT_CD = 6000;
		static constexpr int TRIPLE_CD = 6000;
		static constexpr int SHARP_CD = 6000;
		static constexpr int LL_CD = 9000;
		static constexpr int CONVERT_CD = 18000;
		static constexpr int ENO_CD = 3000;

		// Assume not using B1 or Flare
		static constexpr double F1_POTENCY = 180.0;
		static constexpr double F3_POTENCY = 240.0;
		static constexpr double F4_POTENCY = 300.0;
		static constexpr double B1_POTENCY = 180.0;
		static constexpr double B3_POTENCY = 240.0;
		static constexpr double B4_POTENCY = 260.0;
		static constexpr double T3_POTENCY = 70.0;
		static constexpr double T3_DOT_POTENCY = 40.0;
		static constexpr double TC_POTENCY = 390.0;
		static constexpr double FOUL_POTENCY = 650.0;

		static constexpr double ENO_MULTIPLIER = 1.10;
		static constexpr double MAGICK_AND_MEND_MULTIPLIER = 1.30;
		static constexpr double AF1_MULTIPLIER = 1.40;
		static constexpr double AF2_MULTIPLIER = 1.60;
		static constexpr double AF3_MULTIPLIER = 1.80;
		static constexpr double AF1UI1_MULTIPLIER = 0.90;
		static constexpr double AF2UI2_MULTIPLIER = 0.80;
		static constexpr double AF3UI3_MULTIPLIER = 0.70;

		// MP costs and multipliers
		static constexpr int F1_MP_COST = 1200;
		static constexpr int F3_MP_COST = 2400;
		static constexpr int F4_MP_COST = 1200;
		static constexpr int B1_MP_COST = 480;
		static constexpr int B3_MP_COST = 1440;
		static constexpr int B4_MP_COST = 960;
		static constexpr int T3_MP_COST = 1920;

		int mp = MAX_MP;

		Element element = Element::NE;
		int umbral_hearts = 0;
		bool enochian = false;

		const int base_gcd;
		const int iii_gcd;
		const int iv_gcd;
		const int fast_base_gcd;
		const int fast_iii_gcd;
		const int ll_base_gcd;
		const int ll_iii_gcd;
		const int ll_iv_gcd;
		const int ll_fast_base_gcd;
		const int ll_fast_iii_gcd;

		// server ticks
		Timer mp_timer;
		Timer dot_timer;

		// elemental gauge
		Buff gauge;
		Timer foul_timer;

		// buffs
		Buff swift;
		Buff sharp;
		Buff triple;
		Buff leylines;
		Buff fs_proc;
		Buff tc_proc;
		Buff dot;	// NOT ACTUALLY A BUFF BUT YOU KNOW
					// (value == 1) <=> enochian

		// cooldowns
		Timer eno_cd;
		Timer swift_cd;
		Timer triple_cd;
		Timer sharp_cd;
		Timer leylines_cd;
		Timer convert_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer lock_timer;
		Action casting = Action::NONE;

		// metrics
		long long damage_reward = 0;
		long long total_damage = 0;
		
		BlackMage(Stats stats);

		void start(Rotation* rotation, int time_limit_in_seconds);
		void train();

		void update(int elapsed);
		void step();
		
		void update_mp();
		void update_dot();

		bool is_instant_cast(Action action) const;
		int get_ll_cast_time(int ll_cast_time, int cast_time) const;

		int get_cast_time(Action action) const;
		int get_lock_time(Action action) const;
		int get_gcd_time(Action action) const;

		bool can_use_action(Action action) const;
		void use_action(Action action);
		void end_action();

		int get_mp_cost(Action action) const;
		int get_damage(Action action) const;
		int get_dot_damage() const;
	};
}