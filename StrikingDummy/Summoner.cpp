#include "Summoner.h"
#include "Logger.h"
#include <assert.h>
#include <numeric>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

namespace StrikingDummy
{
	Summoner::Summoner(Stats& stats) :
		Job(stats, SMN_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD)))),
		ea_cd(lround(floor(0.1f * floor(this->stats.ss_multiplier * EA_CD))))
	{
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Summoner::reset()
	{
		timeline = {};

		af_procs = 0;
		ea1_procs = 2;
		ea2_procs = 2;
		r4_procs = 0;
		can_use_bahamut = false;
		can_use_fbt = false;
		can_use_brand = false;

		// server ticks
		dot_timer.reset(tick(rng), false);
		auto_timer.reset(10, false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);

		// buffs
		dwt.reset(0, 0);
		bahamut.reset(0, 0);
		phoenix.reset(0, 0);
		swift.reset(0, 0);
		devotion.reset(0, 0);
		dot_miasma.reset(0, 0);
		dot_bio.reset(0, 0);
		pot.reset(0, 0);

		// cooldowns
		ed_cd.reset(0, true);
		tri_cd.reset(0, true);
		fester_cd.reset(0, true);
		ea1_cd.reset(0, true);
		ea2_cd.reset(0, true);
		enkindle_cd.reset(0, true);
		dwt_cd.reset(0, true);
		akhmorn_cd.reset(0, true);
		swift_cd.reset(0, true);
		devotion_cd.reset(0, true);
		pot_cd.reset(0, true);
		pet_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = Action::NONE;

		// precast
		//timeline.push_event(pot.time);
		//timeline.push_event(pot_cd.time);

		// metrics
		total_damage = 0;
		pot_count = 0;
		total_dot_time = 0;
		
		history.clear();

		update_history();
	}

	void Summoner::update(int elapsed)
	{
		DBG(assert(elapsed > 0));

		// time metrics
		if (dot_miasma.count > 0 && dot_bio.count > 0)
			total_dot_time += elapsed;

		// server ticks
		dot_timer.update(elapsed);
		auto_timer.update(elapsed);

		bool summon = bahamut.count > 0 || phoenix.count > 0;

		// buffs
		dwt.update(elapsed);
		bahamut.update(elapsed);
		phoenix.update(elapsed);
		swift.update(elapsed);
		devotion.update(elapsed);
		dot_miasma.update(elapsed);
		dot_bio.update(elapsed);
		pot.update(elapsed);

		// cooldowns
		ed_cd.update(elapsed);
		tri_cd.update(elapsed);
		fester_cd.update(elapsed);
		ea1_cd.update(elapsed);
		ea2_cd.update(elapsed);
		enkindle_cd.update(elapsed);
		dwt_cd.update(elapsed);
		akhmorn_cd.update(elapsed);
		swift_cd.update(elapsed);
		devotion_cd.update(elapsed);
		pot_cd.update(elapsed);
		pet_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		// 
		if (dot_timer.ready)
			update_dot();
		if (auto_timer.ready)
			update_auto();
		if (ea1_procs < 2 && ea1_cd.ready)
		{
			if (++ea1_procs < 2)
			{
				ea1_cd.reset(ea_cd, false);
				push_event(ea_cd);
			}
		}
		if (ea2_procs < 2 && ea2_cd.ready)
		{
			if (++ea2_procs < 2)
			{
				ea2_cd.reset(ea_cd, false);
				push_event(ea_cd);
			}
		}
		if (summon && bahamut.count == 0 && phoenix.count == 0)
		{
			can_use_brand = false;
			auto_timer.reset(10, false);
			push_event(auto_timer.time);
		}
		if (cast_timer.ready)
			end_action();
		
		update_history();
	}

	void Summoner::update_history()
	{
		actions.clear();
		if (action_timer.ready)
		{
			for (int i = 0; i < NUM_ACTIONS; i++) if (can_use_action(i)) actions.push_back(i);
			
			if (actions.empty() || (actions.size() == 1 && actions[0] == NONE))
				return;

			// state/transition
			if (!history.empty())
			{
				Transition& t = history.back();
				get_state(t.t1);
				t.dt = timeline.time - t.dt;
				t.actions = actions;
			}

			history.emplace_back();
			Transition& t = history.back();
			get_state(t.t0);
			t.reward = 0.0f;
			t.dt = timeline.time;
		}
	}

	void Summoner::update_dot()
	{
		if (dot_miasma.count > 0)
		{
			float damage = get_dot_damage(MIASMA);
			total_damage += damage;
			history.back().reward += damage;
		}
		if (dot_bio.count > 0)
		{
			float damage = get_dot_damage(BIO);
			total_damage += damage;
			history.back().reward += damage;
		}
		dot_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void Summoner::update_auto()
	{
		if (bahamut.count == 0 && phoenix.count == 0)
		{
			float damage = get_pet_damage(NONE);
			total_damage += damage;
			history.back().reward += damage;
		}
		auto_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void Summoner::update_metric(int action)
	{
		/*
		if (!metrics_enabled)
			return;
		switch (action)
		{
		case MANAFONT:
			mf_dist.push_back(timeline.time - mf_last);
			mf_last = timeline.time + MANAFONT_CD;
			break;
		}
		*/
	}

	bool Summoner::is_instant_cast(int action) const
	{
		// for gcds
		return dwt.count > 0 || phoenix.count > 0 || action == R2 || action == R4 || action == BIO || action == EA1 || action == EA2 || swift.count > 0;
	}

	int Summoner::get_cast_time(int action) const
	{
		// for gcds
		if (is_instant_cast(action))
			return 0;
		return base_gcd;
	}

	int Summoner::get_action_time(int action) const
	{
		if (is_instant_cast(action))
			return ANIMATION_LOCK + ACTION_TAX;
		return get_cast_time(action) + ACTION_TAX;
	}

	int Summoner::get_gcd_time() const
	{
		return base_gcd;
	}

	bool Summoner::can_use_action(int action) const
	{
		// only checked if actions aren't locked
		switch (action)
		{
		case NONE:
			return !gcd_timer.ready;
		case R2:
			return gcd_timer.ready && r4_procs == 0 && dwt.count == 0 && phoenix.count == 0;
		case R3:
			return gcd_timer.ready && phoenix.count == 0;
		case R4:
			return gcd_timer.ready && r4_procs > 0 && phoenix.count == 0;
		case MIASMA:
		case BIO:
			return gcd_timer.ready && phoenix.count == 0;
		case ED:
			return ed_cd.ready;
		case TRI:
			return tri_cd.ready;
		case FESTER:
			return af_procs > 0 && fester_cd.ready && dot_miasma.count > 0 && dot_bio.count > 0;
		case EA1:
			return gcd_timer.ready && ea1_procs > 0 && bahamut.count == 0 && phoenix.count == 0;
		case EA2:
			return gcd_timer.ready && ea2_procs > 0 && bahamut.count == 0 && phoenix.count == 0;
		case ENKINDLE:
			return enkindle_cd.ready && bahamut.count == 0 && phoenix.count == 0;
		case DWT:
			return dwt_cd.ready && !can_use_fbt && bahamut.count == 0;
		case DEATHFLARE:
			return dwt.count > 0;
		case BAHAMUT:
			return can_use_bahamut && dwt.count == 0 && pet_cd.ready;
		case AKHMORN:
			return bahamut.count > 0 && akhmorn_cd.ready;
		case FBT:
			return dwt_cd.ready && can_use_fbt && bahamut.count == 0 && pet_cd.ready;
		case FOUNTAIN:
			return phoenix.count > 0 && gcd_timer.ready && !can_use_brand;
		case BRAND:
			return phoenix.count > 0 && gcd_timer.ready && can_use_brand;
		case REVELATION:
			return phoenix.count > 0 && akhmorn_cd.ready;
		case SWIFT:
			return swift_cd.ready;
		case DEVOTION:
			return devotion_cd.ready && bahamut.count == 0 && phoenix.count == 0;
		case POT:
			return pot_cd.ready;
		}
		return false;
	}

	void Summoner::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
			return;
		case R2:
		case R3:
		case R4:
		case MIASMA:
		case BIO:
		case EA1:
		case EA2:
		case FOUNTAIN:
		case BRAND:
			gcd_timer.reset(get_gcd_time(), false);
			cast_timer.reset(get_cast_time(action), false);
			action_timer.reset(get_action_time(action), false);
			casting = action;
			if (cast_timer.time == 0)
				end_action();
			push_event(gcd_timer.time);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			return;
		case ED:
		case TRI:
		case FESTER:
		case ENKINDLE:
		case DEATHFLARE:
		case AKHMORN:
		case REVELATION:
			use_damage_ogcd(action);
			break;
		case DWT:
			can_use_bahamut = true;
			dwt.reset(DWT_DURATION, 1);
			dwt_cd.reset(TRANCE_CD, false);
			tri_cd.reset(0, true);
			push_event(DWT_DURATION);
			push_event(TRANCE_CD);
			break;
		case BAHAMUT:
			can_use_bahamut = false;
			can_use_fbt = true;
			akhmorn_cd.reset(0, true);
			bahamut.reset(BAHAMUT_DURATION, 1);
			push_event(BAHAMUT_DURATION);
			break;
		case FBT:
			can_use_fbt = false;
			if (gcd_timer.time < PHOENIX_DUMB_CD)
			{
				gcd_timer.reset(PHOENIX_DUMB_CD, false);
				push_event(PHOENIX_DUMB_CD);
			}
			phoenix.reset(FBT_DURATION, 1);
			dwt_cd.reset(TRANCE_CD, false);
			tri_cd.reset(0, true);
			akhmorn_cd.reset(0, true);
			push_event(FBT_DURATION);
			push_event(TRANCE_CD);
			break;
		case SWIFT:
			swift.reset(SWIFT_DURATION, 1);
			swift_cd.reset(SWIFT_CD, false);
			push_event(SWIFT_DURATION);
			push_event(SWIFT_CD);
			break;
		case DEVOTION:
			devotion.reset(DEVOTION_DURATION, 1);
			devotion_cd.reset(DEVOTION_CD, false);
			push_event(DEVOTION_DURATION);
			push_event(DEVOTION_CD);
			break;
		case POT:
			pot_count++;
			pot.reset(POT_DURATION, 1);
			pot_cd.reset(POT_CD, false);
			push_event(POT_DURATION);
			push_event(POT_CD);
			action_timer.reset(POTION_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			return;
		}
		// ogcd only
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
		update_metric(action);
	}

	void Summoner::end_action()
	{
		assert(casting != NONE);
		assert(cast_timer.time == 0);
		assert(cast_timer.ready || is_instant_cast(casting));

		//update_metric(casting);

		float damage = get_damage(casting);
		total_damage += damage;
		history.back().reward += damage;

		if (swift.count > 0 && (casting == R3 || casting == MIASMA) && dwt.count == 0)
			swift.reset(0, 0);

		switch (casting)
		{
		case R4:
			r4_procs--;
			break;
		case MIASMA:
			dot_miasma.reset(DOT_DURATION, (1 | (devotion.count > 0 ? 2 : 0) | (pot.count > 0 ? 4 : 0)));
			push_event(DOT_DURATION);
			break;
		case BIO:
			dot_bio.reset(DOT_DURATION, (1 | (devotion.count > 0 ? 2 : 0) | (pot.count > 0 ? 4 : 0)));
			push_event(DOT_DURATION);
			break;
		case EA1:
			r4_procs = std::min(r4_procs + 1, 4);
			ea1_procs--;
			if (ea1_procs == 1)
			{
				ea1_cd.reset(ea_cd, false);
				push_event(ea_cd);
			}
			pet_cd.reset(base_gcd, false);
			push_event(base_gcd);
			break;
		case EA2:
			r4_procs = std::min(r4_procs + 1, 4);
			ea2_procs--;
			if (ea2_procs == 1)
			{
				ea2_cd.reset(ea_cd, false);
				push_event(ea_cd);
			}
			pet_cd.reset(base_gcd, false);
			push_event(base_gcd);
			break;
		case FOUNTAIN:
			can_use_brand = true;
			break;
		case BRAND:
			can_use_brand = false;
			break;
		default:
			break;
		}
		casting = NONE;
		cast_timer.ready = false;
	}

	void Summoner::use_damage_ogcd(int action)
	{
		float damage = get_damage(action);
		total_damage += damage;
		history.back().reward += damage;
		switch (action)
		{
		case ED:
			af_procs = 2;
			ed_cd.reset(ED_CD, false);
			push_event(ED_CD);
			break;
		case TRI:
			dot_miasma.reset(DOT_DURATION, (1 | (devotion.count > 0 ? 2 : 0) | (pot.count > 0 ? 4 : 0)));
			dot_bio.reset(DOT_DURATION, (1 | (devotion.count > 0 ? 2 : 0) | (pot.count > 0 ? 4 : 0)));
			tri_cd.reset(TRI_CD, false);
			push_event(TRI_CD);
			push_event(DOT_DURATION);
			break;
		case FESTER:
			af_procs--;
			fester_cd.reset(FESTER_CD, false);
			push_event(FESTER_CD);
			break;
		case ENKINDLE:
			enkindle_cd.reset(ENKINDLE_CD, false);
			pet_cd.reset(base_gcd, false);
			push_event(ENKINDLE_CD);
			push_event(base_gcd);
			break;
		case DEATHFLARE:
			dwt.reset(0, false);
			break;
		case AKHMORN:
		case REVELATION:
			akhmorn_cd.reset(AKHMORN_CD, false);
			push_event(AKHMORN_CD);
			break;
		}
	}

	float Summoner::get_damage(int action)
	{
		float potency = 0.0f;
		bool summon = bahamut.count > 0 || phoenix.count > 0;
		switch (action)
		{
		case R2:
			potency = R2_POTENCY;
			break;
		case R3:
			potency = R3_POTENCY;
			break;
		case R4:
			potency = R4_POTENCY;
			break;
		case MIASMA:
			potency = MIASMA_POTENCY;
			break;
		case BIO:
			break;
		case ED:
			potency = ED_POTENCY;
			summon = false;
			break;
		case TRI:
			potency = TRI_POTENCY;
			summon = false;
			break;
		case FESTER:
			potency = FESTER_POTENCY;
			summon = false;
			break;
		case DEATHFLARE:
			potency = DEATHFLARE_POTENCY;
			summon = false;
			break;
		case FOUNTAIN:
			potency = FOUNTAIN_POTENCY;
			break;
		case BRAND:
			potency = BRAND_POTENCY;
			break;
		case EA1:
		case EA2:
		case ENKINDLE:
		case AKHMORN:
		case REVELATION:
			return get_pet_damage(action);
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		return potency * stats.potency_multiplier * stats.expected_multiplier * (devotion.count > 0 ? DEVOTION_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER + (summon ? get_pet_damage(BAHAMUT) : 0.0f);
	}
	
	float Summoner::get_dot_damage(int action)
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		if (action == MIASMA)
			return DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * stats.expected_multiplier * ((dot_miasma.count & 2) ? DEVOTION_MULTIPLIER : 1.0f) * ((dot_miasma.count & 4) ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
		return DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * stats.expected_multiplier * ((dot_bio.count & 2) ? DEVOTION_MULTIPLIER : 1.0f) * ((dot_bio.count & 4) ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
	}

	float Summoner::get_pet_damage(int action)
	{
		float potency = 0.0f;
		switch (action)
		{
		case NONE:
			potency = AUTO_POTENCY;
			break;
		case BAHAMUT:
			potency = WYRMWAVE_POTENCY;
			break;
		case EA1:
		case EA2:
			potency = EA_POTENCY;
			break;
		case ENKINDLE:
			potency = ENKINDLE_POTENCY;
			break;
		case AKHMORN:
		case REVELATION:
			potency = AKHMORN_POTENCY;
			break;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		return potency * stats.pet_potency_multiplier * stats.expected_multiplier * (devotion.count > 0 ? DEVOTION_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER * PET_MULTIPLIER;
	}

	void Summoner::get_state(float* state)
	{
		state[0] = af_procs > 0;
		state[1] = af_procs == 1;
		state[2] = af_procs == 2;
		state[3] = ea1_procs > 0;
		state[4] = ea1_procs == 1;
		state[5] = ea1_procs == 2;
		state[6] = ea2_procs > 0;
		state[7] = ea2_procs == 1;
		state[8] = ea2_procs == 2;
		state[9] = r4_procs > 0;
		state[10] = r4_procs == 1;
		state[11] = r4_procs == 2;
		state[12] = r4_procs == 3;
		state[13] = r4_procs == 4;
		state[14] = can_use_bahamut;
		state[15] = can_use_fbt;
		state[16] = can_use_brand;
		state[17] = dwt.count > 0;
		state[18] = dwt.time / (float)DWT_DURATION;
		state[19] = bahamut.count > 0;
		state[20] = bahamut.time / (float)BAHAMUT_DURATION;
		state[21] = phoenix.count > 0;
		state[22] = phoenix.time / (float)FBT_DURATION;
		state[23] = swift.count > 0;
		state[24] = swift.time / (float)SWIFT_DURATION;
		state[25] = devotion.count > 0;
		state[26] = devotion.time / (float)DEVOTION_DURATION;
		state[27] = dot_miasma.count > 0;
		state[28] = dot_miasma.time / (float)DOT_DURATION;
		state[29] = (dot_miasma.count & 2) != 0;
		state[30] = (dot_miasma.count & 4) != 0;
		state[31] = dot_bio.count > 0;
		state[32] = dot_bio.time / (float)DOT_DURATION;
		state[33] = (dot_bio.count & 2) != 0;
		state[34] = (dot_bio.count & 4) != 0;
		state[35] = pot.count > 0;
		state[36] = pot.time / (float)POT_DURATION;
		state[37] = ed_cd.ready;
		state[38] = ed_cd.time / (float)ED_CD;
		state[39] = tri_cd.ready;
		state[40] = tri_cd.time / (float)TRI_CD;
		state[41] = fester_cd.ready;
		state[42] = fester_cd.time / (float)FESTER_CD;
		state[43] = ea1_procs == 2 ? 0.0f : (1.0f - ea1_cd.time / (float)ea_cd);
		state[44] = ea2_procs == 2 ? 0.0f : (1.0f - ea2_cd.time / (float)ea_cd);
		state[45] = enkindle_cd.ready;
		state[46] = enkindle_cd.time / (float)ENKINDLE_CD;
		state[47] = dwt_cd.ready;
		state[48] = dwt_cd.time / (float)TRANCE_CD;
		state[49] = akhmorn_cd.ready;
		state[50] = akhmorn_cd.time / (float)AKHMORN_CD;
		state[51] = swift_cd.ready;
		state[52] = swift_cd.time / (float)SWIFT_CD;
		state[53] = devotion_cd.ready;
		state[54] = devotion_cd.time / (float)DEVOTION_CD;
		state[55] = pot_cd.ready;
		state[56] = pot_cd.time / (float)POT_CD;
		state[57] = pet_cd.ready;
		state[58] = pet_cd.time / (float)base_gcd;
		state[59] = gcd_timer.ready;
		state[60] = gcd_timer.time / (float)base_gcd;
	}

	std::string Summoner::get_info()
	{
		return "\n";
	}
}