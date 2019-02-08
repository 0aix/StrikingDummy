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
			HIGANBANA, TENKA, MIDARE, MEIKYO, KAITEN, SHINTEN, GUREN, HAGAKURE
		};

		static constexpr float SAM_ATTR = 112.0f;
		static constexpr int NUM_ACTIONS = 15;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		static constexpr float BASE_GCD = 2.50f;
		static constexpr float IAI_GCD = 1.80f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int COMBO_DURATION = 1000;
		static constexpr int JINPU_DURATION = 3000;
		static constexpr int SHIFU_DURATION = 3000;
		static constexpr int YUKIKAZE_DURATION = 3000;
		static constexpr int MEIKYO_DURATION = 3000;
		static constexpr int KAITEN_DURATION = 1000;
		static constexpr int DOT_DURATION = 6000;

		static constexpr int MEIKYO_CD = 8000;
		static constexpr int KAITEN_CD = 500;
		static constexpr int SHINTEN_CD = 100;
		static constexpr int HAGAKURE_CD = 4000;
		static constexpr int GUREN_CD = 12000;

		static constexpr float HAKAZE_POTENCY = 150.0f;
		static constexpr float JINPU_POTENCY = 100.0f;
		static constexpr float JINPU_COMBO_POTENCY = 300.0f;
		static constexpr float SHIFU_POTENCY = 100.0f;
		static constexpr float SHIFU_COMBO_POTENCY = 300.0f;
		static constexpr float GEKKO_POTENCY = 100.0f;
		static constexpr float GEKKO_COMBO_POTENCY = 440.0f;
		static constexpr float KASHA_POTENCY = 100.0f;
		static constexpr float KASHA_COMBO_POTENCY = 440.0f;
		static constexpr float YUKIKAZE_POTENCY = 100.0f;
		static constexpr float YUKIKAZE_COMBO_POTENCY = 380.0f;
		static constexpr float SHINTEN_POTENCY = 300.0f;
		static constexpr float GUREN_POTENCY = 800.0f;
		static constexpr float HIGANBANA_POTENCY = 240.0f;
		static constexpr float TENKA_POTENCY = 360.0f;
		static constexpr float MIDARE_POTENCY = 720.0f;
		static constexpr float HIGANBANA_DOT_POTENCY = 35.0f;

		static constexpr float JINPU_MULTIPLIER = 1.10f;
		static constexpr float YUKIKAZE_MULTIPLIER = 1.10f;
		static constexpr float KAITEN_MULTIPLIER = 1.50f;

		static constexpr int KAITEN_COST = 20;
		static constexpr int SHINTEN_COST = 25;
		static constexpr int GUREN_COST = 50;

		static constexpr int MAX_KENKI = 100;

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

		// ticks
		Timer dot_timer;
		Timer auto_timer;

		// buffs
		Buff jinpu_combo;
		Buff shifu_combo;
		Buff gekko_combo;
		Buff kasha_combo;
		Buff yukikaze_combo;
		Buff jinpu;
		Buff shifu;
		Buff yukikaze;
		Buff meikyo;
		Buff kaiten;
		Buff dot;

		// dot buffs
		bool dot_jinpu = false;
		bool dot_yukikaze = false;
		bool dot_kaiten = false;

		Timer meikyo_cd;
		Timer kaiten_cd;
		Timer shinten_cd;
		Timer hagakure_cd;
		Timer guren_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer action_timer;
		int casting;

		// metrics
		int midare_count = 0;

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
		int get_state_size() { return 38; }
		int get_num_actions() { return NUM_ACTIONS; }
	};
}