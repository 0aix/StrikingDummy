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
			MUKU_CHIEF, CELESTIAL_FLIER, GOBLIN_KING, ROROLA,
			SPEAR_THRUST,
			TORNADO_HIT,
			PHANTOM_ARROW, FANTASIA_IMPACT
		};
		
		enum Opener
		{
			GAUGE, NO_GAUGE
		};
		
		static constexpr bool ENABLE_FALCON_TOSS_TALENT = false;
		static constexpr bool ENABLE_SKYFALL_TALENT = true;
		static constexpr bool ENABLE_KAMIKAZE_LUCK_TALENT = true;
		static constexpr bool ENABLE_INSTANT_EDGE_BREAK_TALENT = false;
		static constexpr bool ENABLE_SHARP_ECHO_TALENT = false;
		static constexpr bool ENABLE_BATTLE_CRY_TALENT = false;
		static constexpr bool ENABLE_SPEAR_THRUST_TALENT = true;
		static constexpr bool ENABLE_TORNADO_TALENT = true;
		static constexpr bool ENABLE_MOMENTUM_SURGE_TALENT = true;
		static constexpr bool ENABLE_INSTANT_CRIT_TALENT = true;
		static constexpr bool ENABLE_SHARP_IMPACT_TALENT = false;
		static constexpr bool ENABLE_INSTANT_EDGE_COMBO_TALENT = true;
		static constexpr bool ENABLE_EXP_CRIT_PEN_TALENT = false;
		static constexpr bool ENABLE_LUCK_TORNADO_FACTOR = true;
		static constexpr bool ENABLE_SET_BONUS = true;
		static constexpr bool ENABLE_SET_BONUS_2 = true;

		static constexpr bool ENABLE_PHANTOM_ARROW = true;
		static constexpr bool ENABLE_FANTASIA_IMPACT = false;

		static constexpr bool ENABLE_FANTASIA_EXTRA_BASE_LUCK = false;
		static constexpr bool ENABLE_FANTASIA_EXTRA_STACKS = false;

		static constexpr float FANTASIA_BASE_LUCK = ENABLE_FANTASIA_EXTRA_BASE_LUCK ? 0.04f : 0.01f;
		static constexpr float FANTASIA_LUCK_MULTI = ENABLE_FANTASIA_EXTRA_BASE_LUCK ? 0.0f : 0.10f;
		static constexpr float FANTASIA_STAT_PER = 0.10f;

		static constexpr bool ENABLE_OCEAN_WEAPON = true;
		static constexpr float OCEAN_GALEFORM_BOOST = ENABLE_OCEAN_WEAPON ? 1.06f : 1.0f;
		static constexpr float OCEAN_CRIT = ENABLE_OCEAN_WEAPON ? 0.09f : 0.03f;
		static constexpr float OCEAN_LUCK = ENABLE_OCEAN_WEAPON ? 0.09f : 0.03f;
		static constexpr float OCEAN_HASTE = 0.03f;
		static constexpr float OCEAN_MASTERY = 0.03f;

		static constexpr int GALEFORM_DURATION_FACTOR = 4000;
		static constexpr int MAX_SHARP_STACKS = 6;
		
		static constexpr bool ENABLE_EXPERTISE_DREAM_FACTOR = true;
		static constexpr bool ENABLE_CRIT_MASTERY_FACTOR = false;
		static constexpr bool ENABLE_ALL_ELEMENT_FACTOR = true;
		static constexpr float INSTANT_EDGE_COMBO_DREAM_DMG = 1.0f;

		static constexpr float CRIT_FACTOR_RATE = 1.10f;
		static constexpr float MASTERY_FACTOR_RATE = 0.94f;
		static constexpr float ALL_ELEMENT_FACTOR_BONUS = 212.0f;

		static constexpr float TORNADO_CHANCE_TO_HIT = 0.92f;
		
		static constexpr int SET_GALEFORM_CD_REDUCTION = 200;
		static constexpr float SET_BONUS_WIND_DMG = ENABLE_SET_BONUS_2 ? 0.70f : 0.0f;

		static constexpr float EXPERTISE_DREAM_DMG = (ENABLE_PHANTOM_ARROW ? 0.26f : 0.0f) + (ENABLE_EXPERTISE_DREAM_FACTOR ? 0.07f : 0.0f);
		static constexpr float LUCK_TORNADO_FACTOR_DREAM_DMG = 0.0879f;

		static constexpr float PHANTOM_ARROW_POTENCY = 1.65f * 10.0f;
		static constexpr float PHANTOM_ARROW_DREAM_DMG = 0.65f;
		static constexpr int PHANTOM_ARROW_CD = 7000;
		static constexpr int PHANTOM_ARROW_CD_REDUCTION = 600;
		static constexpr int PHANTOM_ARROW_DOUBLE_CD_REDUCTION = 1200;
		static constexpr float PHANTOM_ARROW_DOUBLE_CD_REDUCTION_CHANCE = 0.30f;
		static constexpr float PHANTOM_ARROW_MASTERY_DMG_CONVERSION = 0.5f;

		static constexpr float FANTASIA_IMPACT_POTENCY = 12.5f * 2.0f * 1.5f;
		static constexpr int FANTASIA_IMPACT_CD = ENABLE_FANTASIA_EXTRA_STACKS ? 10000 : 15000;
		static constexpr int FANTASIA_IMPACT_CD_REDUCTION = 300;
		static constexpr int FANTASIA_IMPACT_MAX_STACKS = ENABLE_FANTASIA_EXTRA_STACKS ? 30 : 20;

		static constexpr bool ENABLE_MUKU_CHIEF = false;
		static constexpr bool ENABLE_GOBLIN_KING = true;
		static constexpr bool ENABLE_CELESTIAL_FLIER = true;
		static constexpr bool ENABLE_ROROLA = false;

		const std::string blm_actions[15] =
		{
			"BASIC_ATTACK", "SKYFALL", "BATTLE_CRY", "TYPHOON_CLEAVE",
			"INSTANT_EDGE", "FALCON_TOSS", "AZURE_SEVER", "SHARP_IMPACT",
			"GALEFORM", "FALL", "WAIT_FOR_GAUGE",
			"MUKU_CHIEF", "CELESTIAL_FLIER", "GOBLIN_KING", "ROROLA"
		};

		static constexpr int NUM_ACTIONS = 15;

		static constexpr int ACTION_TAX = 50;
		
		static constexpr float STAT_MOD = 19976.0f;
		static constexpr float VERS_MOD = 11200.0f;
		static constexpr float ELE_MOD = 6500.0f;
		
		static constexpr float MAX_GAUGE = 130.0f;
		static constexpr float BASE_GAUGE_PER_TICK = 3.0f;
		static constexpr float GALEFORM_GAUGE_PER_TICK = 7.0f;
		static constexpr float ENHANCED_GALEFORM_GAUGE_PER_TICK = 10.5f;
		static constexpr float INSPIRE_GAUGE_PER_TICK = 10.0f;
		static constexpr float GAUGE_PER_SHARP = ENABLE_KAMIKAZE_LUCK_TALENT ? 3.0f : 0.0f;
		static constexpr float TYPHOON_CLEAVE_GAUGE = 100.0f;
		static constexpr float SPEAR_THRUST_GAUGE = 8.0f;

		static constexpr int MAX_IMPACT = 20;
		static constexpr int MAX_FALCON_TOSS_PROCS = ENABLE_FALCON_TOSS_TALENT ? 2 : 1;
		static constexpr int SHARP_IMPACT_SHARP = ENABLE_SHARP_IMPACT_TALENT ? 6 : 3;

		static constexpr int TICK_TIMER = 1000;
		static constexpr int TORNADO_TICK_TIMER = 1000;
		static constexpr int SHARP_DURATION = 10000;
		static constexpr int CHASE_DURATION = 10000;
		static constexpr int INSPIRE_DURATION = 10000;
		static constexpr int TYPHOON_CLEAVE_DURATION = 15000;
		static constexpr int WINDFURY_DURATION = 15000;
		static constexpr int GALEFORM_DURATION = 15000 + GALEFORM_DURATION_FACTOR;
		static constexpr int TEMPESTRIKE_BASE_DURATION = 8000;
		static constexpr int TORNADO_DURATION = 2066;
		static constexpr int DIVINE_HASTE_DURATION = 5000;
		static constexpr int CHASING_STR_DURATION = 10000;
		static constexpr int IMAGINE_DURATION = 20000;

		static constexpr int TYPHOON_CLEAVE_CD = 60000;
		static constexpr int FALCON_TOSS_CD = 23000;
		static constexpr int GALEFORM_CD = 30000;
		static constexpr int SPEAR_THRUST_CD = 2000;
		static constexpr int MUKU_CHIEF_CD = 60000;
		static constexpr int CELESTIAL_FLIER_CD = 80000;
		static constexpr int GOBLIN_KING_CD = 100000;
		static constexpr int ROROLA_CD = 80000;

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
		static constexpr float SPEAR_THRUST_POTENCY = 1.50f;
		static constexpr float TORNADO_POTENCY = 3.50f;
		static constexpr float CELESTIAL_FLIER_POTENCY = 26.25f;
		static constexpr float CELESTIAL_FLIER_ATK = 150.0f;
		static constexpr float MUKU_CHIEF_POTENCY = 17.50f;
		static constexpr float MUKU_CHIEF_ATK = 100.0f;
		static constexpr float GOBLIN_KING_ARMOR_POTENCY = 10.50f;
		static constexpr float GOBLIN_KING_ARMOR_ATK = 60.0f;
		static constexpr float GOBLIN_KING_LUCK_POTENCY = 15.748f;
		static constexpr float GOBLIN_KING_LUCK_ATK = 88.0f;
		static constexpr float GOBLIN_KING_STR_POTENCY = 13.125f;
		static constexpr float GOBLIN_KING_STR_ATK = 75.0f;
		static constexpr float GOBLIN_KING_WIND_POTENCY = 13.12f;
		static constexpr float GOBLIN_KING_WIND_ATK = 70.0f;
		static constexpr float GOBLIN_KING_BOSS_POTENCY = 18.375f;
		static constexpr float GOBLIN_KING_BOSS_ATK = 105.0f;
		static constexpr float ROROLA_POTENCY = 26.249f;
		static constexpr float ROROLA_ATK = 149.0f;

		// Damage bonuses
		static constexpr float EXP_SKILL_DMG = 0.07f;
		static constexpr float VULN_DMG = 0.10f;
		static constexpr float INSTANT_EDGE_BREAK_DMG = ENABLE_INSTANT_EDGE_BREAK_TALENT ? 0.90f : 0.0f;
		static constexpr float WINDFURY_BONUS_DMG = 0.15f * 1.50f; // 0.225f
		static constexpr float GALEFORM_BONUS_DMG = 0.25f;
		static constexpr float TORNADO_BONUS_DMG = ENABLE_TORNADO_TALENT ? 0.30f : 0.00f;
		static constexpr float SPEAR_THRUST_BONUS_DMG = ENABLE_TORNADO_TALENT ? 0.30f : 0.00f;
		static constexpr float CHASING_STEP_DMG = 0.30f;

		// Buffs
		static constexpr float SHARP_ATK = MAX_SHARP_STACKS * 0.06f + (ENABLE_MOMENTUM_SURGE_TALENT ? (0.02f * MAX_SHARP_STACKS) : 0.0f);
		static constexpr float GALEFORM_FLAT_STR = 175.0f; // added before str %
		static constexpr float GALEFORM_STR = 0.38f;
		static constexpr float TEMPESTRIKE_STR = 0.12f;
		static constexpr float CHASING_STR = 0.10f;
		static constexpr float DIVINE_HASTE = 0.01f;
		static constexpr float INSPIRE_HASTE = 0.10f;
		static constexpr float LUCKY_STRIKE_MULTIPLIER = ENABLE_KAMIKAZE_LUCK_TALENT ? 1.50f : 1.00f;
		static constexpr float ENHANCED_MULTIPLIER = 1.50f;

		static constexpr float MUKU_CHIEF_CRIT = 4480.0f;
		static constexpr float MUKU_CHIEF_CRIT_MULTI = 0.40f;
		static constexpr float CELESTIAL_FLIER_HASTE_PERCENT = 0.20f;

		static constexpr float GOBLIN_KING_ARMOR_PEN = 400.0f;
		static constexpr int GOBLIN_KING_ARMOR_PEN_DURATION = 10000;
		static constexpr float GOBLIN_KING_LUCK_BONUS = 3584.0f;
		static constexpr float GOBLIN_KING_LUCK_MULTI = 0.20f;
		static constexpr float GOBLIN_KING_STR_BONUS = 0.20f;
		static constexpr float GOBLIN_KING_WIND_BONUS = 0.16f;
		static constexpr float GOBLIN_KING_BOSS_LUCK_BONUS = 4480.0f;
		static constexpr float GOBLIN_KING_BOSS_LUCK_DMG = 0.32f;
		static constexpr float GOBLIN_KING_BOSS_CHANCE = 0.30f;

		static constexpr float ROROLA_DMG = 0.20f;
		static constexpr float ROROLA_EXTRA_DMG = 0.024f;
		static constexpr int ROROLA_EXTRA_DURATION = 3000;
		static constexpr int ROROLA_MAX_EXTRA_DURATION = 3000;

		// Skill costs
		static constexpr float SKYFALL_GAUGE_COST = 35.0f;
		static constexpr float FALCON_TOSS_GAUGE_COST = ENABLE_FALCON_TOSS_TALENT ? 20.0f : 40.0f;
		static constexpr int INSTANT_EDGE_STACK_COST = ENABLE_INSTANT_EDGE_BREAK_TALENT ? 3 : 2;
		static constexpr int SHARP_IMPACT_STACK_COST = 20;
		static constexpr float SKYFALL_TALENT_CHANCE = 0.15f;
		static constexpr float OTHER_SKYFALL_TALENT_DMG = 0.25f;

		Opener opener;

		float gauge = MAX_GAUGE;
		int sharp = 0;
		int impact = 0;

		bool galeform_active = false;
		float tempestrike_gauge = 0.0f;
		float falcon_gauge = 0.0f;
		int fantasia_stacks = 0;
		int rorola_hits = 0;
		int rorola_stacks = 0;

		bool in_air = false;
		bool azure = false;
		bool prev_falcon_toss = false;
		bool falcon_crit = false;
		bool enhanced_galeform_next = false;
		bool enhanced_skyfall_next = false;
		bool enhanced_instant_edge_next = false;
		bool iec_next = false;

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
		Timer tornado_timer_4;
		Timer tornado_timer_5;
		Timer tornado_timer_6;
		Timer falcon_toss_timer;
		Timer muku_chief_timer;
		Timer rorola_timer;

		int galeform_procs = 0;
		int falcon_toss_procs = 0;
		int muku_chief_procs = 0;
		int rorola_procs = 0;

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
		Buff tornado_4;
		Buff tornado_5;
		Buff tornado_6;
		Buff muku_chief;
		Buff celestial_flier;
		Buff goblin_king_armor_pen;
		Buff goblin_king_luck;
		Buff goblin_king_str;
		Buff goblin_king_wind;
		Buff goblin_king_boss_luck;
		Buff rorola_dmg;
		Buff rorola_extra_dmg;

		// cooldowns
		Timer typhoon_cleave_cd;
		Timer spear_thrust_cd;
		Timer celestial_flier_cd;
		Timer goblin_king_cd;
		Timer phantom_arrow_cd;
		Timer fantasia_impact_cd;

		// actions
		Timer cast_timer;
		Timer action_timer;
		int casting = -1;
		int cast_frame = -1;
		float cast_speed = 1.0f;

		// count metrics
		int tornado_count = 0;
		int skyfall_count = 0;
		int phantom_arrow_count = 0;
		int fantasia_impact_count = 0;

		double total_tornado_damage = 0.0f;
		double total_phantom_arrow_damage = 0.0f;
		double total_fantasia_damage = 0.0f;

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
		int get_state_size() { return 46 + ENABLE_PHANTOM_ARROW * 2 + ENABLE_FANTASIA_IMPACT * 3 +  ENABLE_MUKU_CHIEF * 5 + ENABLE_GOBLIN_KING * 4 + ENABLE_CELESTIAL_FLIER * 4 + ENABLE_ROROLA * 10; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return blm_actions[action]; }
		std::string get_info();
	};
}