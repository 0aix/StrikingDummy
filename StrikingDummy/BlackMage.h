#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct State
	{
		float data[47];
	};

	struct Transition
	{
		State t0;
		State t1;
		int action;
		float reward;
		int dt;
		std::vector<int> actions;
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

		static constexpr float BASE_GCD = 2.50f;
		static constexpr float III_GCD = 3.50f;
		static constexpr float IV_GCD = 2.80f;
		static constexpr float FAST_BASE_GCD = 1.25f;
		static constexpr float FAST_III_GCD = 1.75f;

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
		static constexpr float F1_POTENCY = 180.0f;
		static constexpr float F3_POTENCY = 240.0f;
		static constexpr float F4_POTENCY = 300.0f;
		static constexpr float B1_POTENCY = 180.0f;
		static constexpr float B3_POTENCY = 240.0f;
		static constexpr float B4_POTENCY = 260.0f;
		static constexpr float T3_POTENCY = 70.0f;
		static constexpr float T3_DOT_POTENCY = 40.0f;
		static constexpr float TC_POTENCY = 390.0f;
		static constexpr float FOUL_POTENCY = 650.0f;

		static constexpr float ENO_MULTIPLIER = 1.10f;
		static constexpr float MAGICK_AND_MEND_MULTIPLIER = 1.30f;
		static constexpr float AF1_MULTIPLIER = 1.40f;
		static constexpr float AF2_MULTIPLIER = 1.60f;
		static constexpr float AF3_MULTIPLIER = 1.80f;
		static constexpr float AF1UI1_MULTIPLIER = 0.90f;
		static constexpr float AF2UI2_MULTIPLIER = 0.80f;
		static constexpr float AF3UI3_MULTIPLIER = 0.70f;

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
		int casting = Action::NONE;
		int casting_mp_cost = 0;

		// metrics
		int foul_count = 0;
		int f4_count = 0;

		std::vector<Transition> history;
		int dot_index = -1;

		BlackMage(Stats stats);

		void reset();
		void update(int elapsed);
		void refresh_state();

		void update_mp();
		void update_dot();

		bool is_instant_cast(int action) const;
		int get_ll_cast_time(int ll_cast_time, int cast_time) const;

		int get_cast_time(int action) const;
		int get_lock_time(int action) const;
		int get_gcd_time(int action) const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void end_action();

		int get_mp_cost(int action) const;
		int get_damage(int action) const;
		int get_dot_damage() const;

		void get_state(State& state);

		void* get_history();
		void* get_state();
	};
}