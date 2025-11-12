#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct BlackMage : public Job
	{
		enum Action
		{
			BASIC_ATTACK, SKYFALL, BATTLE_CRY, TYPHOON_CLEAVE,
			INSTANT_EDGE, FALCON_TOSS, AZURE_SEVERER, SHARP_IMPACT,
			GALEFORM, FALL, WAIT_FOR_GAUGE,
			SPEAR_THRUST,
			TORNADO_HIT_1, TORNADO_HIT_2, TORNADO_HIT_3
		};

		enum Opener
		{
			GAUGE, NO_GAUGE
		};


		const std::string blm_actions[11] =
		{
			"BASIC_ATTACK", "SKYFALL", "BATTLE_CRY", "TYPHOON_CLEAVE",
			"INSTANT_EDGE", "FALCON_TOSS", "AZURE_SEVERER", "SHARP_IMPACT",
			"GALEFORM", "FALL", "WAIT_FOR_GAUGE"
		};

		static constexpr int NUM_ACTIONS = 11;

		static constexpr int ACTION_TAX = 100;
		
		static constexpr float STAT_MOD = 4457.0f;
		
		static constexpr float MAX_GAUGE = 130.0f;
		static constexpr float BASE_GAUGE_PER_TICK = 3.0f;
		static constexpr float GALEFORM_GAUGE_PER_TICK = 7.0f;
		static constexpr float ENHANCED_GALEFORM_GAUGE_PER_TICK = 10.5f;
		static constexpr float INSPIRE_GAUGE_PER_TICK = 10.0f;
		static constexpr float GAUGE_PER_SHARP = 3.0f;
		static constexpr float TYPHOON_CLEAVE_GAUGE = 100.0f;
		static constexpr float SPEAR_THRUST_GAUGE = 8.0f;

		static constexpr int MAX_IMPACT = 20;

		static constexpr int TICK_TIMER = 1000;
		static constexpr int SHARP_DURATION = 10000;
		static constexpr int CHASE_DURATION = 10000;
		static constexpr int INSPIRE_DURATION = 10000;
		static constexpr int TYPHOON_CLEAVE_DURATION = 15000;
		static constexpr int WINDFURY_DURATION = 15000;
		static constexpr int GALEFORM_DURATION = 15000;
		static constexpr int TEMPESTRIKE_BASE_DURATION = 8000;
		static constexpr int TORNADO_DURATION = 2800;
		static constexpr int HASTE_DURATION = 5000;
		static constexpr int CHASING_STR_DURATION = 10000;
		static constexpr int SET_BONUS_DMG_DURATION = 5000;

		static constexpr int TYPHOON_CLEAVE_CD = 60000;
		static constexpr int FALCON_TOSS_CD = 23000;
		static constexpr int GALEFORM_CD = 30000;
		static constexpr int SPEAR_THRUST_CD = 2000;

		static constexpr int BASIC_ATTACK_ANIM = 1000;
		static constexpr int SKYFALL_JUMP_ANIM = 1000;
		static constexpr int SKYFALL_ANIM = 1000;
		static constexpr int SKYFALL_ANIM_LOCK = 1000;
		static constexpr int BATTLE_CRY_ANIM = 1000;
		static constexpr int TYPHOON_CLEAVE_ANIM = 1000;
		static constexpr int TYPHOON_CLEAVE_ANIM_LOCK = 1000;
		static constexpr int INSTANT_EDGE_ANIM_JUMP = 1000;
		static constexpr int INSTANT_EDGE_ANIM_1 = 1000;
		static constexpr int INSTANT_EDGE_ANIM_2 = 1000;
		static constexpr int INSTANT_EDGE_ANIM_LOCK = 1000;
		static constexpr int FALCON_TOSS_ANIM_1 = 1000;
		static constexpr int FALCON_TOSS_ANIM_2 = 1000;
		static constexpr int SHARP_IMPACT_GROUND_FIXED_JUMP_ANIM = 1000;
		static constexpr int SHARP_IMPACT_GROUND_ANIM = 1000;
		static constexpr int SHARP_IMPACT_AIR_ANIM = 1000;
		static constexpr int SHARP_IMPACT_ANIM_LOCK = 1000;
		static constexpr int GALEFORM_ANIM = 600;
		static constexpr int FALL_ANIM = 1000;
		static constexpr int FAST_FALL_ANIM = 1000;
		static constexpr int SHORT_FALL_ANIM = 1000;

		// Potencies
		static constexpr float BASIC_ATTACK_POTENCY = 0.42f;
		static constexpr float BASIC_ATTACK_ATK = 120.0f;
		static constexpr float SKYFALL_POTENCY = 2.45f;
		static constexpr float SKYFALL_ATK = 600.0f;
		static constexpr float TYPHOON_CLEAVE_POTENCY = 9.10f;
		static constexpr float TYPHOON_CLEAVE_ATK = 2600.0f;
		static constexpr float GALEFORM_POTENCY = 0.70f;
		static constexpr float GALEFORM_ATK = 200.0f;
		static constexpr float FALCON_TOSS_POTENCY_1 = 1.05f;
		static constexpr float FALCON_TOSS_POTENCY_2 = 7.00f;
		static constexpr float FALCON_TOSS_ATK_1 = 300.0f;
		static constexpr float FALCON_TOSS_ATK_2 = 2000.0f;
		static constexpr float INSTANT_EDGE_POTENCY_1 = 0.64f;
		static constexpr float INSTANT_EDGE_POTENCY_2 = 4.40f;
		static constexpr float INSTANT_EDGE_ATK_1 = 187.0f;
		static constexpr float INSTANT_EDGE_ATK_2 = 1250.0f;
		static constexpr float SHARP_IMPACT_POTENCY = 8.40f;
		static constexpr float SHARP_IMPACT_ATK = 2400.0f;
		static constexpr float SPEAR_THRUST_POTENCY = 1.50f;
		static constexpr float TORNADO_POTENCY_1 = 3.50f;
		static constexpr float TORNADO_POTENCY_2 = 2.80f;
		static constexpr float TORNADO_POTENCY_3 = 2.10f;

		// Damage bonuses
		static constexpr float EXP_SKILL_DMG = 0.07f;
		static constexpr float VULN_DMG = 0.10f;
		static constexpr float INSTANT_EDGE_BREAK_DMG = 0.90f;
		static constexpr float WINDFURY_BONUS_DMG = 0.15f * 1.50f; // 0.225f
		static constexpr float GALEFORM_BONUS_DMG = 0.25f;
		static constexpr float TORNADO_BONUS_DMG = 0.30f;
		static constexpr float SPEAR_THRUST_BONUS_DMG = 0.30f;
		static constexpr float SET_ELE_DMG = 0.10f;
		static constexpr float SET_BONUS_DMG = 0.01f;

		// Buffs
		static constexpr float SHARP_ATK = 0.36f;
		static constexpr float GALEFORM_FLAT_STR = 175.0f; // added before str %
		static constexpr float GALEFORM_STR = 0.38f;
		static constexpr float TEMPESTRIKE_STR = 0.12f;
		static constexpr float CHASING_STR = 0.10f;
		static constexpr float DIVINE_HASTE = 0.01f;
		static constexpr float INSPIRE_HASTE = 0.10f;
		static constexpr float LUCKY_STRIKE_MULTIPLIER = 1.50f;
		static constexpr float ENHANCED_MULTIPLIER = 1.50f;

		// Skill costs
		static constexpr float SKYFALL_GAUGE_COST = 35.0f;
		static constexpr float FALCON_TOSS_GAUGE_COST = 40.0f;
		static constexpr int INSTANT_EDGE_STACK_COST = 3;
		static constexpr int SHARP_IMPACT_STACK_COST = 20;

		float crit_rate;
		float luck_rate;

		Opener opener;

		float gauge = MAX_GAUGE;
		int sharp = 0;
		int impact = 0;

		bool galeform_active = false;
		float tempestrike_gauge = 0.0f;
		float falcon_gauge = 0.0f;

		bool in_air = false;
		bool azure = false;
		bool prev_falcon_toss = false;
		bool enhanced_galeform_next = false;

		// ticks
		Timer gauge_timer;
		Timer galeform_gauge_timer;
		Timer inspire_gauge_timer;
		Timer inspire_sharp_timer;

		// misc timers
		Timer galeform_timer;
		Timer sharp_timer;
		Timer tornado_timer_1;
		Timer tornado_timer_2;
		Timer tornado_timer_3;

		int galeform_procs = 0;

		// buffs
		Buff chasing_step;
		Buff inspire;
		Buff typhoon_cleave;
		Buff windfury;
		Buff galeform;
		Buff tempestrike;
		Buff divine_haste;
		Buff chasing_str;
		Buff set_bonus_dmg;
		Buff tornado_1;
		Buff tornado_2;
		Buff tornado_3;

		// cooldowns
		Timer typhoon_cleave_cd;
		Timer falcon_toss_cd;
		Timer spear_thrust_cd;

		// actions
		Timer cast_timer;
		Timer action_timer;
		int casting = -1;
		int cast_frame = -1;
		float cast_speed = 1.0f;

		// count metrics
		int tornado_count = 0;

		double total_tornado_damage = 0.0f;

		BlackMage(Stats& stats, Opener opener);

		void reset();
		void reset(int gauge_tick);
		void reset(BlackMage& blm);

		void update(int elapsed);
		void update_history();

		void update_gauge();

		float get_cast_speed() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void start_action(int action, int frame, int cast_time, bool start, int offset = 0);
		void end_action();
		void create_tornado();

		float get_damage(int action, int hit = 0);

		void get_state(float* state);
		int get_state_size() { return 55; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return blm_actions[action]; }
		std::string get_info();
	};
}