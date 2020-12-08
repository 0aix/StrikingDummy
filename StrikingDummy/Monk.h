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
			PERFECTBALANCE, BROTHERHOOD, RIDDLEOFFIRE, POT, WAIT
		};

		enum Form
		{
			NORMAL, OPOOPO, RAPTOR, COEURL, PERFECT
		};

		std::string monk_actions[17] =
		{
			"NONE",
			"BOOTSHINE", "TRUESTRIKE", "SNAPPUNCH", "DRAGONKICK", "TWINSNAKES", "DEMOLISH", "SIXSIDEDSTAR",
			"SHOULDERTACKLE", "FORBIDDENCHAKRA", "ELIXIRFIELD", "TORNADOKICK",
			"PERFECTBALANCE", "BROTHERHOOD", "RIDDLEOFFIRE", "POT", "WAIT"
		};

		static constexpr float MONK_ATTR = 110.0f;
		//static constexpr int NUM_ACTIONS = 17;
		static constexpr int NUM_ACTIONS = 16;

		static constexpr int ACTION_TAX = 117;
		static constexpr int ANIMATION_LOCK = 600;
		static constexpr int POTION_LOCK = 1100;

		static constexpr float BASE_GCD = 2.50f;

		static constexpr float BRO_PROC_RATE = 0.20f;

		static constexpr int TICK_TIMER = 3000;
		static constexpr int MEDI_TIMER = 357;
		static constexpr int TWIN_DURATION = 15000;
		static constexpr int DOT_DURATION = 18000;
		static constexpr int ROF_DURATION = 20000;
		static constexpr int BRO_DURATION = 15000;
		static constexpr int POT_DURATION = 30000;

		static constexpr int TACKLE_TIMER = 30000;
		static constexpr int PB_CD = 120000;
		static constexpr int ELIXIR_CD = 30000;
		static constexpr int TK_CD = 45000;
		static constexpr int ROF_CD = 90000;
		static constexpr int BRO_CD = 90000;
		static constexpr int POT_CD = 270000;

		static constexpr float BOOTSHINE_POTENCY = 200.0f;
		static constexpr float LF_BOOTSHINE_POTENCY = 370.0f;
		static constexpr float TRUESTRIKE_POTENCY = 300.0f;
		static constexpr float SNAPPUNCH_POTENCY = 300.0f;
		static constexpr float DRAGONKICK_POTENCY = 230.0f;
		static constexpr float TWINSNAKES_POTENCY = 260.0f;
		static constexpr float DEMOLISH_POTENCY = 110.0f;
		static constexpr float DEMOLISH_DOT_POTENCY = 80.0f;
		static constexpr float SIXSIDEDSTAR_POTENCY = 540.0f;
		static constexpr float FORBIDDENCHAKRA_POTENCY = 340.0f;
		static constexpr float ELIXIRFIELD_POTENCY = 250.0f;
		static constexpr float TORNADOKICK_POTENCY = 400.0f;
		static constexpr float SHOULDERTACKLE_POTENCY = 100.0f;

		static constexpr float FOF_MULTIPLIER = 1.10f;
		static constexpr float TWIN_MULTIPLIER = 1.10f;
		static constexpr float BRO_MULTIPLIER = 1.05f;
		static constexpr float ROF_MULTIPLIER = 1.25f;

		const int base_gcd;
		const int auto_gcd;

		int chakra = 5;

		// ticks
		Timer dot_timer;
		Timer auto_timer;
		Timer bro_timer;

		// buffs
		Form form = NORMAL;
		bool lf = false;
		Buff twin;
		Buff rof;
		Buff bro;
		Buff dot;
		Buff pot;
		int pb_count = 0;

		// dot buffs
		bool dot_twin = false;
		bool dot_bro = false;
		bool dot_rof = false;
		bool dot_pot = false;

		Timer tackle_timer;
		int tackle_procs = 0;

		Timer pb_cd;
		Timer elixir_cd;
		Timer tk_cd;
		Timer rof_cd;
		Timer bro_cd;
		Timer pot_cd;

		// actions
		Timer gcd_timer;
		Timer action_timer;
		Timer medi_timer;

		// metrics
		int sss_count = 0;

		// misc
		float crit_expected_multiplier;

		Monk(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_dot();
		void update_auto();
		void update_medi();

		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void use_damage_action(int action);

		float get_damage(int action) const;
		float get_dot_damage() const;
		float get_auto_damage() const;

		void get_state(float* state);
		int get_state_size() { return 39; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return monk_actions[action]; }
		std::string get_info();
	};
}