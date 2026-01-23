#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct BlackMage : public Job
	{
		enum Action
		{
			BASIC_ATTACK, SKYFALL, BATTLE_CRY, TYPHOON_CLEAVE,
			INSTANT_EDGE, FALCON_TOSS, AZURE_SEVER, SHARP_IMPACT,
			GALEFORM, FALL, WAIT_FOR_GAUGE,
			MUKU_CHIEF, CELESTIAL_FLIER,
			SPEAR_THRUST,
			TORNADO_HIT
		};

		enum Opener
		{
			GAUGE, NO_GAUGE
		};

		static constexpr bool ENABLE_FALCON_TOSS_TALENT = true;
		static constexpr bool ENABLE_SHARP_IMPACT_TALENT = true;
		static constexpr bool ENABLE_SPEAR_THRUST_TALENT = false;
		static constexpr bool ENABLE_TORNADO_TALENT = false;
		static constexpr bool ENABLE_EXP_CRIT_PEN_TALENT = false;
		static constexpr bool ENABLE_BATTLE_CRY_TALENT = true;
		static constexpr bool ENABLE_INSTANT_EDGE_COMBO_TALENT = false;
		static constexpr bool ENABLE_MOMENTUM_SURGE_TALENT = true;
		static constexpr bool ENABLE_LUCK_TORNADO_FACTOR = false;
		static constexpr int GALEFORM_DURATION_FACTOR = 1600;

		const std::string blm_actions[13] =
		{
			"BASIC_ATTACK", "SKYFALL", "BATTLE_CRY", "TYPHOON_CLEAVE",
			"INSTANT_EDGE", "FALCON_TOSS", "AZURE_SEVER", "SHARP_IMPACT",
			"GALEFORM", "FALL", "WAIT_FOR_GAUGE",
			"MUKU_CHIEF", "CELESTIAL_FLIER"
		};

		static constexpr int NUM_ACTIONS = 13;

		static constexpr int ACTION_TAX = 50;
		
		static constexpr float STAT_MOD = 19980.0f;
		static constexpr float VERS_MOD = 11216.0f;
		static constexpr float ELE_MOD = 6500.0f;
		
		static constexpr float MAX_GAUGE = 130.0f;
		static constexpr float BASE_GAUGE_PER_TICK = 3.0f;
		static constexpr float GALEFORM_GAUGE_PER_TICK = 7.0f;
		static constexpr float ENHANCED_GALEFORM_GAUGE_PER_TICK = 10.5f;
		static constexpr float INSPIRE_GAUGE_PER_TICK = 10.0f;
		static constexpr float GAUGE_PER_SHARP = 3.0f;
		static constexpr float TYPHOON_CLEAVE_GAUGE = 100.0f;
		static constexpr float SPEAR_THRUST_GAUGE = ENABLE_SPEAR_THRUST_TALENT ? 8.0f : 0.0f;

		static constexpr int MAX_IMPACT = 20;
		static constexpr int MAX_FALCON_TOSS_PROCS = ENABLE_FALCON_TOSS_TALENT ? 2 : 1;
		static constexpr int SHARP_IMPACT_SHARP = ENABLE_SHARP_IMPACT_TALENT ? 6 : 3;

		static constexpr int TICK_TIMER = 1000;
		static constexpr int SHARP_DURATION = 10000;
		static constexpr int CHASE_DURATION = 10000;
		static constexpr int INSPIRE_DURATION = 10000;
		static constexpr int TYPHOON_CLEAVE_DURATION = 15000;
		static constexpr int WINDFURY_DURATION = 15000;
		static constexpr int GALEFORM_DURATION = 15000 + GALEFORM_DURATION_FACTOR;
		static constexpr int TEMPESTRIKE_BASE_DURATION = 8000;
		static constexpr int TORNADO_DURATION = 2700;
		static constexpr int DIVINE_HASTE_DURATION = 5000;
		static constexpr int CHASING_STR_DURATION = 10000;
		static constexpr int IMAGINE_DURATION = 20000;

		static constexpr int TYPHOON_CLEAVE_CD = 60000;
		static constexpr int FALCON_TOSS_CD = 23000;
		static constexpr int GALEFORM_CD = 30000;
		static constexpr int SPEAR_THRUST_CD = 2000;
		static constexpr int MUKU_CHIEF_CD = 75000;
		static constexpr int CELESTIAL_FLIER_CD = 100000;// 120000;

		static constexpr int BASIC_ATTACK_ANIM = 250;
		static constexpr int SKYFALL_JUMP_ANIM = 535;
		static constexpr int SKYFALL_ANIM = 325;
		static constexpr int SKYFALL_ANIM_LOCK = 655;
		static constexpr int BATTLE_CRY_ANIM = 1000;
		static constexpr int TYPHOON_CLEAVE_ANIM = 1240;
		static constexpr int TYPHOON_CLEAVE_ANIM_LOCK = 485;
		static constexpr int INSTANT_EDGE_ANIM_JUMP = 605;
		static constexpr int INSTANT_EDGE_ANIM_1 = 245;
		static constexpr int INSTANT_EDGE_ANIM_2 = 700;
		static constexpr int INSTANT_EDGE_ANIM_LOCK = 350;
		static constexpr int FALCON_TOSS_ANIM_1 = 630;
		static constexpr int FALCON_TOSS_ANIM_2 = 750;
		static constexpr int SHARP_IMPACT_GROUND_FIXED_JUMP_ANIM = 200;
		static constexpr int SHARP_IMPACT_GROUND_ANIM = 1830;
		static constexpr int SHARP_IMPACT_AIR_ANIM = 1880;
		static constexpr int SHARP_IMPACT_ANIM_LOCK = 500;
		static constexpr int GALEFORM_ANIM = 590;
		static constexpr int FALL_ANIM = 1200;
		static constexpr int FAST_FALL_ANIM = 715;
		static constexpr int SHORT_FALL_ANIM = 540;
		static constexpr int IMAGINE_FIXED_ANIM = 500;
		static constexpr int IMAGINE_ANIM_LOCK = 180;

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
		static constexpr float SHARP_IMPACT_POTENCY = 8.40f * (ENABLE_SHARP_IMPACT_TALENT ? 1.5f : 1.0f);
		static constexpr float SHARP_IMPACT_ATK = 2400.0f * (ENABLE_SHARP_IMPACT_TALENT ? 1.5f : 1.0f);
		static constexpr float SPEAR_THRUST_POTENCY = ENABLE_SPEAR_THRUST_TALENT ? 1.50f : 0.00f;
		static constexpr float TORNADO_POTENCY = 3.50f;

		// Damage bonuses
		static constexpr float EXP_SKILL_DMG = 0.07f;
		static constexpr float VULN_DMG = 0.10f;
		static constexpr float INSTANT_EDGE_BREAK_DMG = 0.90f;
		static constexpr float WINDFURY_BONUS_DMG = 0.15f * 1.50f; // 0.225f
		static constexpr float GALEFORM_BONUS_DMG = 0.25f;
		static constexpr float TORNADO_BONUS_DMG = ENABLE_TORNADO_TALENT ? 0.30f : 0.00f;
		static constexpr float SPEAR_THRUST_BONUS_DMG = ENABLE_TORNADO_TALENT ? 0.30f : 0.00f;

		// Buffs
		static constexpr float SHARP_ATK = 0.36f + ENABLE_MOMENTUM_SURGE_TALENT ? 0.12f : 0.0f;
		static constexpr float GALEFORM_FLAT_STR = 175.0f; // added before str %
		static constexpr float GALEFORM_STR = 0.38f;
		static constexpr float TEMPESTRIKE_STR = 0.12f;
		static constexpr float CHASING_STR = 0.10f;
		static constexpr float DIVINE_HASTE = 0.01f;
		static constexpr float INSPIRE_HASTE = 0.10f;
		static constexpr float LUCKY_STRIKE_MULTIPLIER = 1.50f;
		static constexpr float ENHANCED_MULTIPLIER = 1.50f;
		static constexpr float MUKU_CHIEF_CRIT = 4032.0f;
		static constexpr float MUKU_CHIEF_CRIT_MULTI = 0.36f;
		static constexpr float CELESTIAL_FLIER_HASTE_PERCENT = 0.16f;// 0.14f;

		// Skill costs
		static constexpr float SKYFALL_GAUGE_COST = 35.0f;
		static constexpr float FALCON_TOSS_GAUGE_COST = ENABLE_FALCON_TOSS_TALENT ? 20.0f : 40.0f;
		static constexpr int INSTANT_EDGE_STACK_COST = 3;
		static constexpr int SHARP_IMPACT_STACK_COST = 20;

		float luck_rate;
		float luck_multi;

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
		bool falcon_crit = false;
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
		Timer falcon_toss_timer;
		Timer muku_chief_timer;

		int galeform_procs = 0;
		int falcon_toss_procs = 0;
		int muku_chief_procs = 0;

		// buffs
		Buff chasing_step;
		Buff inspire;
		Buff typhoon_cleave;
		Buff windfury;
		Buff galeform;
		Buff tempestrike;
		Buff divine_haste;
		Buff chasing_str;
		Buff tornado_1;
		Buff tornado_2;
		Buff tornado_3;
		Buff muku_chief;
		Buff celestial_flier;

		// cooldowns
		Timer typhoon_cleave_cd;
		Timer spear_thrust_cd;
		Timer celestial_flier_cd;

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
		int get_state_size() { return 58; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return blm_actions[action]; }
		std::string get_info();
	};
}