#pragma once

#include "Job.h"

namespace StrikingDummy
{/*
	struct State
	{
		float data[50];
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
	*/
	struct Mimu : public Job
	{
		enum Action
		{
			NONE,
			BOOTSHINE, TRUESTRIKE, SNAPPUNCH, DRAGONKICK, TWINSNAKES, DEMOLISH, 
			FISTSOFWIND, FISTSOFIRE,
			INTERNALRELEASE, PERFECTBALANCE, BROTHERHOOD, STEELPEAK, HOWLINGFIST, FORBIDDENCHAKRA, ELIXIRFIELD, TORNADOKICK,
			RIDDLEOFWIND, RIDDLEOFIRE, SHOULDERTACKLE, WINDTACKLE, FIRETACKLE
		};

		enum Form
		{
			NORMAL, OPOOPO, RAPTOR, COEURL
		};

		enum Fists
		{
			BARE, WIND, FIRE
		};

		static constexpr int NUM_ACTIONS = 22;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		static constexpr float BASE_GCD = 2.50f;

		static constexpr float MEDITATION_PROC_RATE = 0.50f;
		static constexpr float BRO_PROC_RATE = 0.30f;
		static constexpr float IR_CRIT_RATE = 0.30f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int FORM_DURATION = 1000;
		static constexpr int GL_DURATION = 1600;
		static constexpr int TWIN_DURATION = 1500;
		static constexpr int DK_DURATION = 1500;
		static constexpr int IR_DURATION = 1500;
		static constexpr int DOT_DURATION = 1800;
		static constexpr int ROW_DURATION = 1000;
		static constexpr int ROF_DURATION = 2000;
		static constexpr int BRO_DURATION = 1500;

		static constexpr int IR_CD = 6000;
		static constexpr int FISTS_CD = 300;
		static constexpr int TACKLE_CD = 3000;
		static constexpr int STEEL_CD = 4000;
		static constexpr int HOWLING_CD = 6000;
		static constexpr int PB_CD = 6000;
		static constexpr int CHAKRA_CD = 500;
		static constexpr int ELIXIR_CD = 3000;
		static constexpr int TK_CD = 1000;
		static constexpr int ROF_CD = 9000;
		static constexpr int BRO_CD = 9000;

		static constexpr float BOOT_POTENCY = 140.0f;
		static constexpr float TRUE_POTENCY = 180.0f;
		static constexpr float SNAP_POTENCY = 170.0f;
		static constexpr float TWIN_POTENCY = 130.0f;
		static constexpr float DEMO_POTENCY = 70.0f;
		static constexpr float DOT_POTENCY = 50.0f;
		static constexpr float TACKLE_POTENCY = 100.0f;
		static constexpr float STEEL_POTENCY = 150.0f;
		static constexpr float HOWLING_POTENCY = 210.0f;
		static constexpr float DRAGON_POTENCY = 140.0f;
		static constexpr float CHAKRA_POTENCY = 250.0f;
		static constexpr float ELIXIR_POTENCY = 220.0f;
		static constexpr float TK_POTENCY = 429.0f;
		static constexpr float HOWLING_POTENCY = 210.0f;
		static constexpr float WINDTACKLE_POTENCY = 65.0f;
		static constexpr float FIRETACKLE_POTENCY = 130.0f;
		static constexpr float RIDDLEOFWIND_POTENCY = 210.0f;

		static constexpr float FOF_MULTIPLIER = 1.06f;
		static constexpr float TWIN_MULTIPLIER = 1.10f;
		static constexpr float DK_MULTIPLIER = 1.10f;
		static constexpr float BRO_MULTIPLIER = 1.05f;
		static constexpr float GL1_MULTIPLIER = 1.10f;
		static constexpr float GL2_MULTIPLIER = 1.20f;
		static constexpr float GL3_MULTIPLIER = 1.30f;

		const int base_gcd;
		const int gl1_gcd;
		const int gl2_gcd;
		const int gl3_gcd;
		const int base_fire_gcd;
		const int gl1_fire_gcd;
		const int gl2_fire_gcd;
		const int gl3_fire_gcd;

		Fists fists = Fists::BARE;
		int chakra = 0;

		// ticks
		Timer dot_timer;
		Timer auto_timer;

		// buffs
		Buff form;
		Buff gl;
		Buff twin;
		Buff ir;
		Buff dk;
		Buff pb;
		Buff row;
		Buff rof;
		Buff bro;
		Buff dot;
	
		Timer swift_cd;
		Timer triple_cd;
		Timer sharp_cd;
		Timer leylines_cd;
		Timer convert_cd;
		Timer eno_cd;

		// actions
		Timer gcd_timer;
		Timer lock_timer;

		// metrics
		long long total_damage = 0;
		int foul_count = 0;
		int f4_count = 0;
		int b4_count = 0;

		std::vector<Transition> history;

		Mimu(Stats stats);

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