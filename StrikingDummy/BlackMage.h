#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct BlackMage : public Job
	{
		enum Action
		{
			NONE,
			B1, B3, B4, F1, F3, F4, T3, XENO, DESPAIR, PARADOX,
			SWIFT, TRIPLE, SHARP, LEYLINES, MANAFONT, TRANSPOSE, AMPLIFIER,
			LUCID, WAIT_FOR_MP,
			POT, F3P_OFF
		};

		enum Element
		{
			NE, UI, AF
		};

		enum Opener
		{
			NO_OPENER, PRE_F3, PRE_B3, PRE_T3, PRE_LL_F3, PRE_LL_B3
		};

		enum ActionSet
		{
			FULL, NO_B4, STANDARD
		};

		const std::string blm_actions[22] =
		{
			"NONE",
			"B1", "B3", "B4", "F1", "F3", "F4", "T3", "XENO", "DESPAIR", "PARADOX",
			"SWIFT", "TRIPLE", "SHARP", "LEYLINES", "MANAFONT", "TRANSPOSE", "AMPLIFIER",
			"LUCID", "WAIT_FOR_MP",
			"HQ_TINCTURE_OF_INTELLIGENCE", "F3P OFF"
		};

		static constexpr int NUM_ACTIONS = 22;

		static constexpr float BLM_ATTR = 115.0f;

		static constexpr int ACTION_TAX = 117;
		static constexpr int CAST_LOCK = 500;
		static constexpr int ANIMATION_LOCK = 600;
		static constexpr int POTION_LOCK = 1100;
		static constexpr int LATENCY = 100;

		// Assume only in UI3 and AF3
		static constexpr int MAX_MP = 10000;
		static constexpr int MP_PER_TICK = 200;			// 2% per tick
		static constexpr int MP_PER_TICK_UI1 = 3200;	// 32% per tick in UI1
		static constexpr int MP_PER_TICK_UI2 = 4700;	// 47% per tick in UI2
		static constexpr int MP_PER_TICK_UI3 = 6200;	// 62% per tick in UI3
		static constexpr int MANAFONT_MP = 3000;		// 30%
		static constexpr int LUCID_MP = 550;			// 5.5%

		static constexpr float BASE_GCD = 2.50f;
		static constexpr float III_GCD = 3.50f;
		static constexpr float IV_GCD = 2.80f;
		static constexpr float DESPAIR_GCD = 3.00f;

		static constexpr float TC_PROC_RATE = 0.10f;
		static constexpr float FS_PROC_RATE = 0.40f;

		static constexpr int TICK_TIMER = 3000;
		static constexpr int XENO_TIMER = 30000;
		static constexpr int GAUGE_DURATION = 15000;
		static constexpr int SWIFT_DURATION = 10000;
		static constexpr int TRIPLE_DURATION = 15000;
		static constexpr int SHARP_DURATION = 30000;
		static constexpr int FS_DURATION = 30000;
		static constexpr int TC_DURATION = 40000;
		static constexpr int TC_REFRESH_DURATION = 39960; // 39.96s as taken from packet data
		static constexpr int LL_DURATION = 30000;
		static constexpr int DOT_DURATION = 30000;
		static constexpr int DOT_TRAVEL_DURATION = 1000;
		static constexpr int LUCID_DURATION = 21000;
		static constexpr int POT_DURATION = 30000;

		static constexpr int SWIFT_CD = 60000;
		static constexpr int TRIPLE_CD = 60000;
		static constexpr int SHARP_CD = 30000;
		static constexpr int LL_CD = 120000;
		static constexpr int MANAFONT_CD = 120000;
		static constexpr int TRANSPOSE_CD = 5000;
		static constexpr int LUCID_CD = 60000;
		static constexpr int POT_CD = 270000;
		static constexpr int AMPLIFIER_CD = 120000;

		// Assume not using Flare
		static constexpr float F1_POTENCY = 180.0f;
		static constexpr float F3_POTENCY = 260.0f;
		static constexpr float F4_POTENCY = 310.0f;
		static constexpr float B1_POTENCY = 180.0f;
		static constexpr float B3_POTENCY = 260.0f;
		static constexpr float B4_POTENCY = 310.0f;
		static constexpr float T3_POTENCY = 50.0f;
		static constexpr float T3_DOT_POTENCY = 35.0f;
		static constexpr float TC_POTENCY = 400.0f;
		static constexpr float XENO_POTENCY = 760.0f;
		static constexpr float DESPAIR_POTENCY = 340.0f;
		static constexpr float PARADOX_POTENCY = 500.0f;

		static constexpr float ENO_MULTIPLIER = 1.20f;
		static constexpr float MAGICK_AND_MEND_MULTIPLIER = 1.30f;
		static constexpr float AF1_MULTIPLIER = 1.40f;
		static constexpr float AF2_MULTIPLIER = 1.60f;
		static constexpr float AF3_MULTIPLIER = 1.80f;
		static constexpr float AF1UI1_MULTIPLIER = 0.90f;
		static constexpr float AF2UI2_MULTIPLIER = 0.80f;
		static constexpr float AF3UI3_MULTIPLIER = 0.70f;

		// MP costs and multipliers
		static constexpr int F1_MP_COST = 800;
		static constexpr int F3_MP_COST = 2000;
		static constexpr int F4_MP_COST = 800;
		static constexpr int B1_MP_COST = 400;
		static constexpr int B3_MP_COST = 800;
		static constexpr int B4_MP_COST = 800;
		static constexpr int T3_MP_COST = 400;
		static constexpr int DESPAIR_MP_COST = 800;
		static constexpr int PARADOX_MP_COST = 1600;

		const int base_gcd;
		const int iii_gcd;
		const int iv_gcd;
		const int despair_gcd;
		const int fast_base_gcd;
		const int fast_iii_gcd;
		const int ll_base_gcd;
		const int ll_iii_gcd;
		const int ll_iv_gcd;
		const int ll_despair_gcd;
		const int ll_fast_base_gcd;
		const int ll_fast_iii_gcd;

		Opener opener;
		ActionSet action_set;

		int mp = MAX_MP;

		Element element = Element::NE;
		int umbral_hearts = 0;
		bool enochian = false;
		bool paradox = false;
		bool t3p = false;

		// ticks
		Timer mp_timer;
		Timer dot_timer;
		Timer lucid_timer;
		int mp_wait = 0;

		bool skip_lucid_tick = false;
		bool skip_transpose_tick = false;

		// misc timers
		Buff gauge;
		Timer xeno_timer;
		Timer sharp_timer;
		Timer triple_timer;
		Timer dot_travel_timer;

		int xeno_procs = 0;
		int sharp_procs = 0;
		int triple_procs = 0;
		int dot_travel;

		// buffs
		Buff swift;
		Buff sharp;
		Buff triple;
		Buff leylines;
		Buff fs_proc;
		Buff tc_proc;
		Buff dot; // (value & 2) <=> enochian; (value & 4) <=> pot
		Buff lucid;
		Buff pot;

		// cooldowns
		Timer swift_cd;
		Timer leylines_cd;
		Timer manafont_cd;
		Timer transpose_cd;
		Timer lucid_cd;
		Timer pot_cd;
		Timer amplifier_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer action_timer;
		int casting = Action::NONE;

		// count metrics
		int xeno_count = 0;
		int f1_count = 0;
		int f4_count = 0;
		int b1_count = 0;
		int b3_count = 0;
		int b4_count = 0;
		int t3_count = 0;
		int despair_count = 0;
		int transpose_count = 0;
		int lucid_count = 0;
		int pot_count = 0;
		int total_dot_time = 0;

		double total_f4_damage = 0.0f;
		double total_desp_damage = 0.0f;
		double total_xeno_damage = 0.0f;
		double total_t3_damage = 0.0f;
		double total_dot_damage = 0.0f;

		// distribution metrics tracking how long buffs fall off for before being reapplied
		bool dist_metrics_enabled = false;

		std::vector<int> t3_dist;
		std::vector<int> t3p_dist;
		std::vector<int> swift_dist;
		std::vector<int> triple_dist;
		std::vector<int> sharp_dist;
		std::vector<int> ll_dist;
		std::vector<int> mf_dist;
		int t3_last = 0;
		int swift_last = 0;
		int triple_last = 0;
		int sharp_last = 0;
		int ll_last = 0;
		int mf_last = 0;

		BlackMage(Stats& stats, Opener opener, ActionSet action_set);

		void reset();
		void reset(int mp_tick, int lucid_tick, int dot_tick);
		void reset(BlackMage& blm);

		void update(int elapsed);
		void update_history();

		void update_mp();
		void update_dot();
		void update_lucid();

		void update_metric(int action, float damage = 0.0f);

		bool is_instant_cast(int action) const;
		int get_ll_cast_time(int ll_cast_time, int cast_time) const;

		int get_cast_time(int action) const;
		int get_action_time(int action) const;
		int get_gcd_time(int action) const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void end_action();

		int get_mp_cost(int action, bool is_end_action = false) const;
		float get_damage(int action);
		float get_dot_damage();

		void get_state(float* state);
		int get_state_size() { return 64; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return blm_actions[action]; }
		std::string get_info();
	};
}