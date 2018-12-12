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
		std::priority_queue<int, std::vector<int>, std::greater<int>> events;
		int time = 0;

		int next_event();
		void push_event(int offset);
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

		double potency_multiplier;
		double expected_multiplier;

		void calculate_stats();
	};

	struct State
	{
		float mp;
		float ui;
		float af;
		float umbral_hearts;
		float enochian;
		float mp_tick;
		float dot_tick;
		float gauge;
		float g1;
		float g2;
		float g3;
		float gauge_time;
		float foul_proc;
		float foul_time;
		float swift;
		float swift_time;
		float sharp;
		float sharp_time;
		float triple_procs;
		float triple_time;
		float leylines;
		float ll_time;
		float fs_proc;
		float fs_time;
		float tc_proc;
		float tc_time;
		float dot_ticking;
		float dot_time;
		float dot_enochian;
		float swift_ready;
		float swift_cd;
		float triple_ready;
		float triple_cd;
		float sharp_ready;
		float sharp_cd;
		float leylines_ready;
		float leylines_cd;
		float convert_ready;
		float convert_cd;
		float eno_ready;
		float eno_cd;
		float gcd_ready;
		float gcd_time;
		float casting;
		float cast_time;
		float lock_ready; // can use ogcds
		float lock_time;
		float b1_casting;
		float b3_casting;
		float b4_casting;
		float f1_casting;
		float f3_casting;
		float f4_casting;
		float t3_casting;
		float foul_casting;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast_b1;
		float can_cast[15];
	};

	struct Transition
	{
		State t0;
		State t1;
		int action;
		double reward;
		bool terminal;
	};

	struct Job
	{
		Job(Stats stats) : stats(stats)
		{
			this->stats.calculate_stats();
		}

		virtual void start(Rotation* rotation, int time_limit_in_seconds) = 0;
		virtual void train() = 0;
		virtual void get_state(State& state) = 0;
		virtual void reset() = 0;
		
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
		bool training = false;
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
		
		static constexpr float BASE_GCD = 2.50;
		static constexpr float III_GCD = 3.50;
		static constexpr float IV_GCD = 2.80;
		static constexpr float FAST_BASE_GCD = 1.25;
		static constexpr float FAST_III_GCD = 1.75;

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
		static constexpr float F1_POTENCY = 180.0;
		static constexpr float F3_POTENCY = 240.0;
		static constexpr float F4_POTENCY = 300.0;
		static constexpr float B1_POTENCY = 180.0;
		static constexpr float B3_POTENCY = 240.0;
		static constexpr float B4_POTENCY = 260.0;
		static constexpr float T3_POTENCY = 70.0;
		static constexpr float T3_DOT_POTENCY = 40.0;
		static constexpr float TC_POTENCY = 390.0;
		static constexpr float FOUL_POTENCY = 650.0;

		static constexpr float ENO_MULTIPLIER = 1.10;
		static constexpr float MAGICK_AND_MEND_MULTIPLIER = 1.30;
		static constexpr float AF1_MULTIPLIER = 1.40;
		static constexpr float AF2_MULTIPLIER = 1.60;
		static constexpr float AF3_MULTIPLIER = 1.80;
		static constexpr float AF1UI1_MULTIPLIER = 0.90;
		static constexpr float AF2UI2_MULTIPLIER = 0.80;
		static constexpr float AF3UI3_MULTIPLIER = 0.70;

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
		Timer swift_cd;
		Timer triple_cd;
		Timer sharp_cd;
		Timer leylines_cd;
		Timer convert_cd;
		Timer eno_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer lock_timer;
		Action casting = Action::NONE;
		int casting_mp_cost = 0;

		bool useable[NUM_ACTIONS];
		std::vector<int> useable_actions;

		// misc
		long long total_damage = 0;

		std::vector<State> history;
		int last_action_state = -1;
		int last_dot_state = -1;

		BlackMage(Stats stats);

		void start(Rotation* rotation, int time_limit_in_seconds);
		void train();
		void reset();

		void update(int elapsed);
		
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

		void get_state(State& state);
	};
}