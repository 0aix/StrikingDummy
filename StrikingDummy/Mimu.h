#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct Mimu : public Job
	{
		enum Action
		{
			NONE,
			BOOTSHINE, TRUESTRIKE, SNAPPUNCH, DRAGONKICK, TWINSNAKES, DEMOLISH, 
			FISTSOFWIND, FISTSOFFIRE,
			INTERNALRELEASE, PERFECTBALANCE, BROTHERHOOD, STEELPEAK, HOWLINGFIST, FORBIDDENCHAKRA, ELIXIRFIELD, TORNADOKICK,
			RIDDLEOFWIND, RIDDLEOFFIRE, WINDTACKLE, FIRETACKLE, WAIT
		};

		enum Form
		{
			NORMAL, OPOOPO, RAPTOR, COEURL, PERFECT
		};

		enum Fists
		{
			WIND, FIRE
		};

		static constexpr float MIMU_ATTR = 110.0f;
		static constexpr int NUM_ACTIONS = 22;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		static constexpr float BASE_GCD = 2.50f;

		static constexpr float MEDITATION_PROC_RATE = 0.50f;
		static constexpr float BRO_PROC_RATE = 0.30f;
		static constexpr float IR_CRIT_RATE = 0.30f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int FORM_DURATION = 1000 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int GL_DURATION = 1600 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int TWIN_DURATION = 1500 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int DK_DURATION = 1500 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int IR_DURATION = 1500 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int DOT_DURATION = 1800;
		static constexpr int ROW_DURATION = 1000 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int ROF_DURATION = 2000 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int BRO_DURATION = 1500 + ANIMATION_LOCK + ACTION_TAX;

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
		static constexpr float DEMO_DOT_POTENCY = 50.0f;
		static constexpr float STEEL_POTENCY = 150.0f;
		static constexpr float HOWLING_POTENCY = 210.0f;
		static constexpr float DRAGON_POTENCY = 140.0f;
		static constexpr float CHAKRA_POTENCY = 250.0f;
		static constexpr float ELIXIR_POTENCY = 220.0f;
		static constexpr float TK_POTENCY = 429.0f;
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
		static constexpr float ROF_MULTIPLIER = 1.30f;
		static constexpr float GL_MULTIPLIER[] = { 1.0f, GL1_MULTIPLIER, GL2_MULTIPLIER, GL3_MULTIPLIER };

		const int base_gcd;
		const int gl1_base_gcd;
		const int gl2_base_gcd;
		const int gl3_base_gcd;
		const int fire_gcd;
		const int gl1_fire_gcd;
		const int gl2_fire_gcd;
		const int gl3_fire_gcd;
		const int auto_gcd;
		const int gl1_auto_gcd;
		const int gl2_auto_gcd;
		const int gl3_auto_gcd;

		Fists fists = Fists::WIND;
		int chakra = 5;

		// ticks
		Timer dot_timer;
		Timer auto_timer;

		// buffs
		Buff form;
		Buff gl;
		Buff twin;
		Buff ir;
		Buff dk;
		Buff row;
		Buff rof;
		Buff bro;
		Buff dot;

		// dot buffs...
		int dot_gl = 0;
		bool dot_fof = false;
		bool dot_twin = false;
		bool dot_dk = false;
		bool dot_bro = false;
		bool dot_rof = false;
		bool dot_ir = false;

		Timer ir_cd;
		Timer fists_cd;
		Timer tackle_cd;
		Timer steel_cd;
		Timer howling_cd;
		Timer pb_cd;
		Timer chakra_cd;
		Timer elixir_cd;
		Timer tk_cd;
		Timer rof_cd;
		Timer bro_cd;

		// actions
		Timer gcd_timer;
		Timer action_timer;

		// metrics
		int tk_count = 0;

		// misc
		float ir_expected_multiplier;
		float crit_expected_multiplier;

		Mimu(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_dot();
		void update_auto();
		
		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void use_damage_action(int action);

		float get_damage(int action) const;
		float get_dot_damage() const;
		float get_auto_damage() const;

		void get_state(float* state);
		int get_state_size() { return 60; }
		int get_num_actions() { return NUM_ACTIONS; }
	};
}