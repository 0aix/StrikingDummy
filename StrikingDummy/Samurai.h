#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct Samurai : public Job
	{
		enum Action
		{
			NONE,
			HAKAZE, JINPU, SHIFU, GEKKO, KASHA, YUKIKAZE,
			HIGANBANA, MIDARE, TSUBAME,
			MEIKYO, KAITEN, SHINTEN, SENEI, HAGAKURE, IKISHOTEN, SHOHA,
			POT
		};

		std::string sam_actions[18] =
		{
			"NONE",
			"HAKAZE", "JINPU", "SHIFU", "GEKKO", "KASHA", "YUKIKAZE",
			"HIGANBANA", "MIDARE", "TSUBAME",
			"MEIKYO", "KAITEN", "SHINTEN", "SENEI", "HAGAKURE", "IKISHOTEN", "SHOHA", 
			"HQ_TINCTURE_OF_STRENGTH"
		};

		static constexpr float SAM_ATTR = 112.0f;
		static constexpr int NUM_ACTIONS = 18;

		static constexpr int ACTION_TAX = 117;
		static constexpr int ANIMATION_LOCK = 600;
		static constexpr int POTION_LOCK = 1100;

		static constexpr float BASE_GCD = 2.50f;
		static constexpr float IAI_GCD = 1.30f;

		static constexpr int TICK_TIMER = 3000;
		static constexpr int JINPU_DURATION = 40000;
		static constexpr int SHIFU_DURATION = 40000;
		static constexpr int DOT_DURATION = 60000;
		static constexpr int POT_DURATION = 30000;

		static constexpr int MEIKYO_CD = 55000;
		static constexpr int HAGAKURE_CD = 5000;
		static constexpr int SENEI_CD = 120000;
		static constexpr int IKISHOTEN_CD = 60000;
		static constexpr int TSUBAME_CD = 60000;
		static constexpr int POT_CD = 270000;

		static constexpr float HAKAZE_POTENCY = 200.0f;
		static constexpr float JINPU_POTENCY = 320.0f;
		static constexpr float SHIFU_POTENCY = 320.0f;
		static constexpr float GEKKO_POTENCY = 480.0f;
		static constexpr float KASHA_POTENCY = 480.0f;
		static constexpr float YUKIKAZE_POTENCY = 360.0f;
		static constexpr float SHINTEN_POTENCY = 320.0f;
		static constexpr float SENEI_POTENCY = 1100.0f;
		static constexpr float HIGANBANA_POTENCY = 250.0f;
		static constexpr float MIDARE_POTENCY = 800.0f;
		static constexpr float TSUBAME_POTENCY = 1200.0f;
		static constexpr float SHOHA_POTENCY = 400.0f;
		static constexpr float HIGANBANA_DOT_POTENCY = 40.0f;

		static constexpr float JINPU_MULTIPLIER = 1.13f;
		static constexpr float KAITEN_MULTIPLIER = 1.50f;

		static constexpr int KAITEN_COST = 20;
		static constexpr int SHINTEN_COST = 25;
		static constexpr int SENEI_COST = 50;

		static constexpr int MAX_KENKI = 100;
		static constexpr int HAGAKURE_KENKI = 10;
		static constexpr int IKISHOTEN_KENKI = 50;
		
		static constexpr int MAX_MEDITATION = 3;

		const int base_gcd;
		const int iai_gcd;
		const int shifu_base_gcd;
		const int shifu_iai_gcd;
		const int auto_gcd;
		const int shifu_auto_gcd;

		int kenki = 0;
		bool setsu = false;
		bool getsu = false;
		bool ka = false;
		int meditation = 0;
		int meikyo = 0;
		bool can_jinpu = false;
		bool can_shifu = false;
		bool can_gekko = false;
		bool can_kasha = false;
		bool can_yukikaze = false;
		bool can_kaiten = true;
		bool can_shinten = true;
		bool can_tsubame = false;
		bool kaiten = false;

		// ticks
		Timer dot_timer;
		Timer auto_timer;

		// buffs
		Buff jinpu;
		Buff shifu;
		Buff dot;
		Buff pot;

		// dot buffs
		bool dot_jinpu = false;
		bool dot_kaiten = false;
		bool dot_pot = false;

		Timer meikyo_cd;
		Timer hagakure_cd;
		Timer senei_cd;
		Timer ikishoten_cd;
		Timer tsubame_cd;
		Timer pot_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer action_timer;
		int casting;

		// metrics
		int midare_count = 0;
		int higanbana_count = 0;
		int tsubame_count = 0;
		int kaiten_count = 0;
		int senei_count = 0;
		int shinten_count = 0;
		int hagakure_count = 0;
		int shoha_count = 0;
		int pot_count = 0;

		Samurai(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_dot();
		void update_auto();

		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void use_damage_action(int action);
		void end_action();

		float get_damage(int action) const;
		float get_dot_damage() const;
		float get_auto_damage() const;

		void get_state(float* state);
		int get_state_size() { return 43; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return sam_actions[action]; }
		std::string get_info();
	};
}