#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct Monk : public Job
	{
		enum Action
		{
			NONE,
			BOOTSHINE, TRUESTRIKE, SNAPPUNCH, DRAGONKICK, TWINSNAKES, DEMOLISH, SIXSIDEDSTAR,
			SHOULDERTACKLE, FORBIDDENCHAKRA, ELIXIRFIELD, TORNADOKICK,
			FISTSOFWIND, FISTSOFFIRE,
			PERFECTBALANCE, BROTHERHOOD, RIDDLEOFFIRE, ANATMAN, WAIT
		};

		enum Form
		{
			NORMAL, OPOOPO, RAPTOR, COEURL, PERFECT
		};

		enum Fists
		{
			WIND, FIRE
		};

		std::string monk_actions[19] =
		{
			"NONE",
			"BOOTSHINE", "TRUESTRIKE", "SNAPPUNCH", "DRAGONKICK", "TWINSNAKES", "DEMOLISH", "SIXSIDEDSTAR",
			"SHOULDERTACKLE", "FORBIDDENCHAKRA", "ELIXIRFIELD", "TORNADOKICK",
			"FISTSOFWIND", "FISTSOFFIRE",
			"PERFECTBALANCE", "BROTHERHOOD", "RIDDLEOFFIRE", "ANATMAN", "WAIT"
		};

		static constexpr float MONK_ATTR = 110.0f;
		static constexpr int NUM_ACTIONS = 19;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		static constexpr float BASE_GCD = 2.50f;

		static constexpr float MEDITATION_PROC_RATE = 0.70f;
		static constexpr float BRO_PROC_RATE = 0.30f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int FORM_DURATION = 1000;
		static constexpr int GL_DURATION = 1600;
		static constexpr int LF_DURATION = 3000;
		static constexpr int TWIN_DURATION = 1500;
		static constexpr int DOT_DURATION = 1800;
		static constexpr int ROF_DURATION = 2000;
		static constexpr int BRO_DURATION = 1500;

		static constexpr int FISTS_CD = 300;
		static constexpr int TACKLE_TIMER = 3000;
		static constexpr int PB_CD = 12000;
		static constexpr int ELIXIR_CD = 3000;
		static constexpr int TK_CD = 1000;
		static constexpr int ROF_CD = 9000;
		static constexpr int BRO_CD = 9000;
		static constexpr int ANATMAN_CD = 6000;

		static constexpr float BOOTSHINE_POTENCY = 150.0f;
		static constexpr float LF_BOOTSHINE_POTENCY = 300.0f;
		static constexpr float TRUESTRIKE_POTENCY = 240.0f;
		static constexpr float SNAPPUNCH_POTENCY = 230.0f;
		static constexpr float DRAGONKICK_POTENCY = 200.0f;
		static constexpr float TWINSNAKES_POTENCY = 170.0f;
		static constexpr float DEMOLISH_POTENCY = 90.0f;
		static constexpr float DEMOLISH_DOT_POTENCY = 65.0f;
		static constexpr float SIXSIDEDSTAR_POTENCY = 400.0f;
		static constexpr float FORBIDDENCHAKRA_POTENCY = 370.0f;
		static constexpr float ELIXIRFIELD_POTENCY = 200.0f;
		static constexpr float TORNADOKICK_POTENCY = 430.0f;
		static constexpr float SHOULDERTACKLE_POTENCY = 100.0f;

		static constexpr float FOF_MULTIPLIER = 1.10f;
		static constexpr float TWIN_MULTIPLIER = 1.10f;
		static constexpr float BRO_MULTIPLIER = 1.05f;
		static constexpr float GL1_MULTIPLIER = 1.10f;
		static constexpr float GL2_MULTIPLIER = 1.20f;
		static constexpr float GL3_MULTIPLIER = 1.30f;
		static constexpr float GL4_MULTIPLIER = 1.40f;
		static constexpr float ROF_MULTIPLIER = 1.30f;
		static constexpr float GL_MULTIPLIER[] = { 1.0f, GL1_MULTIPLIER, GL2_MULTIPLIER, GL3_MULTIPLIER, GL4_MULTIPLIER };

		const int base_gcd;
		const int gl1_base_gcd;
		const int gl2_base_gcd;
		const int gl3_base_gcd;
		const int gl4_base_gcd;
		const int fire_gcd;
		const int gl1_fire_gcd;
		const int gl2_fire_gcd;
		const int gl3_fire_gcd;
		const int gl4_fire_gcd;
		const int auto_gcd;
		const int gl1_auto_gcd;
		const int gl2_auto_gcd;
		const int gl3_auto_gcd;
		const int gl4_auto_gcd;

		Fists fists = Fists::FIRE;
		int chakra = 5;

		// ticks
		Timer dot_timer;
		Timer auto_timer;
		Timer anatman_timer;

		// buffs
		Buff form;
		Buff gl;
		Buff lf;
		Buff twin;
		Buff rof;
		Buff bro;
		Buff dot;

		bool anatman;
		bool skip_anatman_tick;

		// dot buffs
		int dot_gl = 0;
		bool dot_fof = false;
		bool dot_twin = false;
		bool dot_bro = false;
		bool dot_rof = false;

		Timer tackle_timer;
		int tackle_procs = 0;

		Timer fists_cd;
		Timer pb_cd;
		Timer elixir_cd;
		Timer tk_cd;
		Timer rof_cd;
		Timer bro_cd;
		Timer anatman_cd;

		// actions
		Timer gcd_timer;
		Timer action_timer;

		// metrics
		int tk_count = 0;
		int sss_count = 0;
		int anatman_count = 0;

		// misc
		float crit_expected_multiplier;

		Monk(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_dot();
		void update_auto();
		void update_anatman();

		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void use_damage_action(int action);

		float get_damage(int action) const;
		float get_dot_damage() const;
		float get_auto_damage() const;

		void get_state(float* state);
		int get_state_size() { return 52; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return monk_actions[action]; }
		std::string get_info();
	};
}