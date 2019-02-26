#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct Machinist : public Job
	{
		enum Action
		{
			NONE,
			SPLIT, SLUG, HOT, CLEAN, COOLDOWN,
			GAUSS_ROUND, RICOCHET,
			RELOAD, REASSEMBLE, QUICK_RELOAD, RAPIDFIRE, WILDFIRE, GAUSS_BARREL, STABILIZER,
			HYPERCHARGE, OVERDRIVE,
			FLAMETHROWER_CAST, FLAMETHROWER_TICK
		};

		static constexpr float MCH_ATTR = 115.0f;
		static constexpr int NUM_ACTIONS = 19;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;

		static constexpr float BASE_GCD = 2.50f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int TURRET_TIMER = 300;
		static constexpr int BONUS_DURATION = 1000;
		static constexpr int REASSEMBLE_DURATION = 2000;
		static constexpr int HOT_DURATION = 6000;
		static constexpr int RAPIDFIRE_DURATION = 1500;
		static constexpr int WILDFIRE_DURATION = 1000 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int HYPERCHARGE_DURATION = 2000;
		static constexpr int VULN_DURATION = 1000;
		static constexpr int FLAMETHROWER_DURATION = 1000 + ANIMATION_LOCK + ACTION_TAX;
		static constexpr int OVERHEAT_DURATION = 1000;

		static constexpr int RELOAD_CD = 3000;
		static constexpr int REASSEMBLE_CD = 6000;
		static constexpr int QUICK_RELOAD_CD = 1500;
		static constexpr int RAPIDFIRE_CD = 6000;
		static constexpr int WILDFIRE_CD = 6000;
		static constexpr int GAUSS_ROUND_CD = 1500;
		static constexpr int BARREL_CD = 1000;
		static constexpr int HYPERCHARGE_CD = 12000;
		static constexpr int RICOCHET_CD = 6000;
		static constexpr int STABILIZER_CD = 6000;
		static constexpr int OVERDRIVE_CD = 12000;
		static constexpr int FLAMETHROWER_CD = 6000;
		static constexpr int TURRET_CD = 3000;

		static constexpr float SPLIT_POTENCY = 160.0f;
		static constexpr float HEATED_SPLIT_POTENCY = 190.0f;
		static constexpr float BONUS_SLUG_POTENCY = 200.0f;
		static constexpr float HEATED_BONUS_SLUG_POTENCY = 230.0f;
		static constexpr float BONUS_CLEAN_POTENCY = 240.0f;
		static constexpr float HEATED_BONUS_CLEAN_POTENCY = 270.0f;
		static constexpr float HOT_POTENCY = 120.0f;
		static constexpr float TURRET_POTENCY = 80.0f;
		static constexpr float HYPERCHARGE_POTENCY = 120.0f;
		static constexpr float GAUSS_ROUND_POTENCY = 210.0f;
		static constexpr float RICOCHET_POTENCY = 320.0f;
		static constexpr float HEATED_COOLDOWN_POTENCY = 230.0f;
		static constexpr float OVERDRIVE_POTENCY = 800.0f;
		static constexpr float FLAMETHROWER_POTENCY = 60.0f;
		static constexpr float AMMO_POTENCY = 30.0f;

		static constexpr float ACTION_MULTIPLIER = 1.20f;
		static constexpr float HOT_MULTIPLIER = 1.10f;
		static constexpr float VULN_MULTIPLIER = 1.05f;
		static constexpr float WILDFIRE_MULTIPLIER = 1.25f;
		static constexpr float GAUSS_MULTIPLIER = 1.05f;
		static constexpr float OVERHEAT_MULTIPLIER = 1.20f;

		static constexpr float BONUS_PROBABILITY = 0.50f;

		static constexpr int FT_TICK = 100;
		static constexpr int MAX_HEAT = 100;

		const int base_gcd;
		const int auto_gcd;
		const int rapid_gcd = 150;

		float crit_expected_multiplier;

		int heat = 0;
		int ammo = 3;
		bool gauss = true;
		bool burning = false;

		// ticks
		Timer auto_timer;
		Timer turret_timer;	

		// buffs
		Buff slug_bonus;
		Buff clean_bonus;
		Buff reassemble;
		Buff hot;
		Buff rapidfire;
		Buff wildfire;
		Buff overheat;
		Buff hypercharge;
		Buff flamethrower;
		Buff vuln;

		// cooldowns
		Timer reload_cd;
		Timer reassemble_cd;
		Timer quick_reload_cd;
		Timer rapidfire_cd;
		Timer wildfire_cd;
		Timer barrel_cd;
		Timer gauss_round_cd;
		Timer hypercharge_cd;
		Timer ricochet_cd;
		Timer stabilizer_cd;
		Timer flamethrower_cd;
		Timer overdrive_cd;
		Timer turret_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer action_timer;
		int casting;

		// metrics

		Machinist(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_auto();
		void update_turret();

		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void use_damage_action(int action);
		void end_action();

		float get_damage(int action) const;
		float get_auto_damage() const;
		float get_turret_damage() const;

		void get_state(float* state);
		int get_state_size() { return 56; }
		int get_num_actions() { return NUM_ACTIONS; }
	};
}