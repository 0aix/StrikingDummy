#pragma once

#include "Job.h"

namespace StrikingDummy
{
	struct Summoner : public Job
	{
		enum Action
		{
			NONE,
			R2, R3, R4, MIASMA, BIO, ED, TRI, FESTER,
			EA1, EA2, ENKINDLE, DWT, DEATHFLARE, BAHAMUT, AKHMORN, FBT, FOUNTAIN, BRAND, REVELATION,
			SWIFT, DEVOTION, POT
		};

		std::string smn_actions[23] =
		{
			"NONE",
			"R2", "R3", "R4", "MIASMA", "BIO", "ED", "TRI-DISASTER", "FESTER",
			"EA1", "EA2", "ENKINDLE", "DWT", "DEATHFLARE", "BAHAMUT", "AKH MORN", "FBT", "FOUNTAIN OF FIRE", "BRAND OF PURGATORY", "REVELATION",
			"SWIFT", "DEVOTION", "HQ_GRADE_2_TINCTURE_OF_INTELLIGENCE"
		};

		static constexpr float SMN_ATTR = 115.0f;

		static constexpr int NUM_ACTIONS = 23;

		static constexpr int ACTION_TAX = 10;
		static constexpr int ANIMATION_LOCK = 60;
		static constexpr int POTION_LOCK = 110;

		static constexpr float BASE_GCD = 2.50f;
		static constexpr float EA_CD = 30.0f;

		static constexpr int TICK_TIMER = 300;
		static constexpr int DWT_DURATION = 1500;
		static constexpr int BAHAMUT_DURATION = 2000;
		static constexpr int FBT_DURATION = 2000;
		static constexpr int SWIFT_DURATION = 1000;
		static constexpr int DEVOTION_DURATION = 1500;
		static constexpr int POT_DURATION = 3000;
		static constexpr int DOT_DURATION = 3000;
		
		static constexpr int ED_CD = 3000;
		static constexpr int TRI_CD = 5000;
		static constexpr int FESTER_CD = 500;
		static constexpr int ENKINDLE_CD = 12000;
		static constexpr int TRANCE_CD = 5500;
		static constexpr int AKHMORN_CD = 1000;
		static constexpr int SWIFT_CD = 6000;
		static constexpr int DEVOTION_CD = 18000;
		static constexpr int POT_CD = 27000;
		static constexpr int PHOENIX_DUMB_CD = 130;

		static constexpr float R2_POTENCY = 160.0f;
		static constexpr float R3_POTENCY = 200.0f;
		static constexpr float R4_POTENCY = 300.0f;
		static constexpr float MIASMA_POTENCY = 50.0f;
		static constexpr float DOT_POTENCY = 50.0f;
		static constexpr float ED_POTENCY = 100.0f;
		static constexpr float TRI_POTENCY = 300.0f;
		static constexpr float FESTER_POTENCY = 300.0f;
		static constexpr float EA_POTENCY = 250.0f;
		static constexpr float ENKINDLE_POTENCY = 400.0f;
		static constexpr float DEATHFLARE_POTENCY = 400.0f;
		static constexpr float AKHMORN_POTENCY = 650.0f;
		static constexpr float WYRMWAVE_POTENCY = 150.0f;
		static constexpr float FOUNTAIN_POTENCY = 250.0f;
		static constexpr float BRAND_POTENCY = 350.0f;
		static constexpr float AUTO_POTENCY = 80.0f;

		static constexpr float MAGICK_AND_MEND_MULTIPLIER = 1.30f;
		static constexpr float DEVOTION_MULTIPLIER = 1.05f;
		static constexpr float PET_MULTIPLIER = 0.80f;

		const int base_gcd;
		const int ea_cd;

		int af_procs = 0;
		int ea1_procs = 2;
		int ea2_procs = 2;
		int r4_procs = 0;
		bool can_use_bahamut = false;
		bool can_use_fbt = false;
		bool can_use_brand = false;

		// ticks
		Timer dot_timer;
		Timer auto_timer;

		// buffs
		Buff dwt;
		Buff bahamut;
		Buff phoenix;
		Buff swift;
		Buff devotion;
		Buff dot_miasma;	// (value & 2) <=> devotion; (value & 4) <=> pot
		Buff dot_bio;		// (value & 2) <=> devotion; (value & 4) <=> pot
		Buff pot;

		// cooldowns
		Timer ed_cd;
		Timer tri_cd;
		Timer fester_cd;
		Timer ea1_cd;
		Timer ea2_cd;
		Timer enkindle_cd;
		Timer dwt_cd;
		Timer akhmorn_cd;
		Timer swift_cd;
		Timer devotion_cd;
		Timer pot_cd;
		Timer pet_cd;

		// actions
		Timer gcd_timer;
		Timer cast_timer;
		Timer action_timer;
		int casting = Action::NONE;

		// count metrics
		int pot_count = 0;
		int total_dot_time = 0;

		// distribution metrics
		bool metrics_enabled = false;
		//std::vector<int> t3_dist;
		//int t3_last = 0;
		//float total_f4_damage = 0.0f;
		//float total_desp_damage = 0.0f;

		Summoner(Stats& stats);

		void reset();
		void update(int elapsed);
		void update_history();

		void update_dot();
		void update_auto();

		void update_metric(int action);

		bool is_instant_cast(int action) const;

		int get_cast_time(int action) const;
		int get_action_time(int action) const;
		int get_gcd_time() const;

		bool can_use_action(int action) const;
		void use_action(int action);
		void end_action();
		void use_damage_ogcd(int action);

		float get_damage(int action);
		float get_dot_damage(int action);
		float get_pet_damage(int action);

		void get_state(float* state);
		int get_state_size() { return 61; }
		int get_num_actions() { return NUM_ACTIONS; }
		std::string get_action_name(int action) { return smn_actions[action]; }
		std::string get_info();
	};
}