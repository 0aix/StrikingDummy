#include "BlackMage.h"
#include "Logger.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <iostream>
#include <Eigen/Core>
#include <fstream>
using namespace Eigen;

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

namespace StrikingDummy
{
	std::vector<int> int_range(BlackMage::NUM_ACTIONS);

	std::string action_names[] =
	{
		"NONE",
		"B1", "B3", "B4", "F1", "F3", "F4", "T3", "FOUL",
		"SWIFT", "TRIPLE", "SHARP", "LEYLINES", "CONVERT", "ENOCHIAN"
	};

	BlackMage::BlackMage(Stats stats) : 
		Job(stats),
		base_gcd(lround(floor(floor(this->stats.ss_multiplier * BASE_GCD) / 10.0))),
		iii_gcd(lround(floor(floor(this->stats.ss_multiplier * III_GCD) / 10.0))),
		iv_gcd(lround(floor(floor(this->stats.ss_multiplier * IV_GCD) / 10.0))),
		fast_base_gcd(lround(floor(floor(this->stats.ss_multiplier * FAST_BASE_GCD) / 10.0))),
		fast_iii_gcd(lround(floor(floor(this->stats.ss_multiplier * FAST_III_GCD) / 10.0))),
		ll_base_gcd(lround(floor(0.85 * floor(this->stats.ss_multiplier * BASE_GCD) / 10.0))),
		ll_iii_gcd(lround(floor(0.85 * floor(this->stats.ss_multiplier * III_GCD) / 10.0))),
		ll_iv_gcd(lround(floor(0.85 * floor(this->stats.ss_multiplier * IV_GCD) / 10.0))),
		ll_fast_base_gcd(lround(floor(0.85 * floor(this->stats.ss_multiplier * FAST_BASE_GCD) / 10.0))),
		ll_fast_iii_gcd(lround(floor(0.85 * floor(this->stats.ss_multiplier * FAST_III_GCD) / 10.0)))
	{
		std::iota(int_range.begin(), int_range.end(), 0);
		actions.reserve(NUM_ACTIONS);
		reset();
		seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	}

	void BlackMage::reset()
	{
		timeline = {};

		mp = MAX_MP;

		element = Element::NE;
		umbral_hearts = 0;
		enochian = false;

		// server ticks
		mp_timer.reset(tick(rng), false);
		dot_timer.reset(tick(rng), false);
		timeline.push_event(mp_timer.time);
		timeline.push_event(dot_timer.time);

		// elemental gauge
		gauge.reset(0, 0);
		foul_timer.reset(0, false);

		// buffs
		swift.reset(0, 0);
		sharp.reset(0, 0);
		triple.reset(0, 0);
		leylines.reset(0, 0);
		fs_proc.reset(0, 0);
		tc_proc.reset(0, 0);
		dot.reset(0, 0);

		// cooldowns		
		swift_cd.reset(0, true);
		triple_cd.reset(0, true);
		sharp_cd.reset(0, true);
		leylines_cd.reset(0, true);
		convert_cd.reset(0, true);
		eno_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		lock_timer.reset(0, true);
		casting = Action::NONE;
		casting_mp_cost = 0;

		// metrics
		total_damage = 0;
		foul_count = 0;
		f4_count = 0;
		b4_count = 0;
		t3_count = 0;
		
		history.clear();

		refresh_state();
	}

	void BlackMage::update(int elapsed)
	{
		DBG(assert(elapsed > 0));

		// server ticks
		mp_timer.update(elapsed);
		dot_timer.update(elapsed);

		// elemental gauge
		gauge.update(elapsed);
		foul_timer.update(elapsed);

		// buffs
		swift.update(elapsed);
		sharp.update(elapsed);
		triple.update(elapsed);
		leylines.update(elapsed);
		fs_proc.update(elapsed);
		tc_proc.update(elapsed);
		dot.update(elapsed);

		// cooldowns
		swift_cd.update(elapsed);
		triple_cd.update(elapsed);
		sharp_cd.update(elapsed);
		leylines_cd.update(elapsed);
		convert_cd.update(elapsed);
		eno_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		cast_timer.update(elapsed);
		lock_timer.update(elapsed);

		// 
		if (mp_timer.ready)
			update_mp();
		if (dot_timer.ready)
			update_dot();
		if (element != Element::NE && gauge.count == 0)
		{
			element = Element::NE;
			umbral_hearts = 0;
			enochian = false;
			foul_timer.time = 0;
		}
		if (enochian && foul_timer.time == 0)
		{
			foul_timer.time = FOUL_TIMER;
			push_event(FOUL_TIMER);
		}
		if (cast_timer.ready)
			end_action();
		
		refresh_state();
	}

	void BlackMage::refresh_state()
	{
		actions.clear();
		if (lock_timer.ready)
		{
			std::remove_copy_if(int_range.cbegin(), int_range.cend(), std::back_inserter(actions), [this](int i) { return !this->can_use_action(i); });
			
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

	void BlackMage::update_mp()
	{
		if (element != Element::AF)
		{
			switch (gauge.count)
			{
			case 0:
				mp += MP_PER_TICK;
				break;
			case 1:
				mp += MP_PER_TICK_UI1;
				break;
			case 2:
				mp += MP_PER_TICK_UI2;
				break;
			case 3:
				mp += MP_PER_TICK_UI3;
			}
			if (mp > MAX_MP)
				mp = MAX_MP;
		}
		mp_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void BlackMage::update_dot()
	{
		if (dot.count > 0)
		{
			int damage = get_dot_damage();
			total_damage += damage;
			history.back().reward += damage;
			if (prob(rng) < TC_PROC_RATE)
			{
				tc_proc.reset(TC_DURATION, 1);
				push_event(TC_DURATION);
			}
		}
		dot_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	bool BlackMage::is_instant_cast(int action) const
	{
		// for gcds
		return swift.count == 1 || triple.count > 0 || (action == F3 && fs_proc.count > 0) || (action == T3 && tc_proc.count > 0);
	}

	int BlackMage::get_ll_cast_time(int ll_cast_time, int cast_time) const
	{
		return (leylines.count == 1 && ll_cast_time < leylines.time) ? ll_cast_time : cast_time;
	}

	int BlackMage::get_cast_time(int action) const
	{
		// for gcds
		if (is_instant_cast(action))
			return 0;

		switch (action)
		{
		case B1:
			if (element == AF && gauge.count == 3)
				return get_ll_cast_time(ll_fast_base_gcd, fast_base_gcd);
			return get_ll_cast_time(ll_base_gcd, base_gcd);
		case B3:
			if (element == AF && gauge.count == 3)
				return get_ll_cast_time(ll_fast_iii_gcd, fast_iii_gcd);
			return get_ll_cast_time(ll_iii_gcd, iii_gcd);
		case B4:
			return get_ll_cast_time(ll_iv_gcd, iv_gcd);
		case F1:
			if (element == UI && gauge.count == 3)
				return get_ll_cast_time(ll_fast_base_gcd, fast_base_gcd);
			return get_ll_cast_time(ll_base_gcd, base_gcd);
		case F3:
			if (element == UI && gauge.count == 3)
				return get_ll_cast_time(ll_fast_iii_gcd, fast_iii_gcd);
			return get_ll_cast_time(ll_iii_gcd, iii_gcd);
		case F4:
			return get_ll_cast_time(ll_iv_gcd, iv_gcd);
		case T3:
		case FOUL:
			return get_ll_cast_time(ll_base_gcd, base_gcd);
		}
		return 99999;
	}

	int BlackMage::get_lock_time(int action) const
	{
		if (is_instant_cast(action))
			return ANIMATION_LOCK + CASTER_TAX;
		return get_cast_time(action) + CASTER_TAX;
	}

	int BlackMage::get_gcd_time(int action) const
	{
		if (is_instant_cast(action))
			return leylines.count > 0 ? ll_base_gcd : base_gcd;
		return get_ll_cast_time(ll_base_gcd, base_gcd);
	}

	bool BlackMage::can_use_action(int action) const
	{
		// only checked if actions aren't locked
		switch (action)
		{
		case NONE:
			if (!gcd_timer.ready)
				return true;
			//return true;
			return !(get_mp_cost(B1) <= mp || get_mp_cost(B3) <= mp || get_mp_cost(T3) <= mp || get_mp_cost(F1) <= mp || get_mp_cost(F3) <= mp || foul_timer.ready ||
				(element == UI && enochian && get_cast_time(B4) < gauge.time && get_mp_cost(B4) <= mp) ||
				(element == AF && enochian && get_cast_time(F4) < gauge.time && get_mp_cost(F4) <= mp));
		case B1:
			return gcd_timer.ready && get_mp_cost(B1) <= mp;
		case B3:
			return gcd_timer.ready && get_mp_cost(B3) <= mp;
		case B4:
			return gcd_timer.ready && element == UI && enochian && get_cast_time(B4) < gauge.time && get_mp_cost(B4) <= mp;
		case F1:
			return gcd_timer.ready && get_mp_cost(F1) <= mp;
		case F3:
			return gcd_timer.ready && get_mp_cost(F3) <= mp;
		case F4:
			return gcd_timer.ready && element == AF && enochian && get_cast_time(F4) < gauge.time && get_mp_cost(F4) <= mp;
		case T3:
			return gcd_timer.ready && get_mp_cost(T3) <= mp;
		case FOUL:
			return gcd_timer.ready && foul_timer.ready;
		case SWIFT:
			return swift_cd.ready;
		case TRIPLE:
			return triple_cd.ready;
		case SHARP:
			return sharp_cd.ready;
		case LEYLINES:
			return leylines_cd.ready;
		case CONVERT:
			return convert_cd.ready;
		case ENOCHIAN:
			return !enochian && eno_cd.ready && element != Element::NE;
		}
		return false;
	}

	void BlackMage::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
			return;
		case B1:
		case B3:
		case B4:
		case F1:
		case F3:
		case F4:
		case T3:
		case FOUL:
			if (action == FOUL)
				foul_count++;
			else if (action == F4)
				f4_count++;
			else if (action == B4)
				b4_count++;
			else if (action == T3)
				t3_count++;
			gcd_timer.reset(get_gcd_time(action), false);
			cast_timer.reset(get_cast_time(action), false);
			lock_timer.reset(get_lock_time(action), false);
			casting = action;
			casting_mp_cost = get_mp_cost(action);
			assert(casting_mp_cost <= mp);
			if (cast_timer.time == 0)
				end_action();
			push_event(gcd_timer.time);
			push_event(cast_timer.time);
			push_event(lock_timer.time);
			return;
		case SWIFT:
			swift.reset(SWIFT_DURATION, 1);
			swift_cd.reset(SWIFT_CD, false);
			push_event(SWIFT_DURATION);
			push_event(SWIFT_CD);
			break;
		case TRIPLE:
			triple.reset(TRIPLE_DURATION, 3);
			triple_cd.reset(TRIPLE_CD, false);
			push_event(TRIPLE_DURATION);
			push_event(TRIPLE_CD);
			break;
		case SHARP:
			sharp.reset(SHARP_DURATION, 1);
			sharp_cd.reset(SHARP_CD, false);
			push_event(SHARP_DURATION);
			push_event(SHARP_CD);
			break;
		case LEYLINES:
			leylines.reset(LL_DURATION, 1);
			leylines_cd.reset(LL_CD, false);
			push_event(LL_DURATION);
			push_event(LL_CD);
			break;
		case CONVERT:
			mp = std::min(mp + CONVERT_MP, MAX_MP);
			convert_cd.reset(CONVERT_CD, false);
			push_event(CONVERT_CD);
			break;
		case ENOCHIAN:
			if (!enochian)
			{
				foul_timer.time = FOUL_TIMER;
				push_event(FOUL_TIMER);
			}
			enochian = true;
			eno_cd.reset(ENO_CD, false);
			push_event(ENO_CD);
		}
		// ogcd only
		lock_timer.reset(ANIMATION_LOCK + CASTER_TAX, false);
		push_event(lock_timer.time);
	}

	void BlackMage::end_action()
	{
		assert(casting != NONE);
		assert(cast_timer.time == 0);
		assert(cast_timer.ready || is_instant_cast(casting));

		mp -= casting_mp_cost;

		assert(mp >= 0);

		int damage = get_damage(casting);

		total_damage += damage;

		history.back().reward += damage;

		if (casting == F3 && fs_proc.count > 0);
		// firestarter doesn't deplete swift or triple
		else if (casting == T3 && tc_proc.count > 0);
		// thundercloud doesn't deplete swift or triple
		else if (swift.count > 0)
			swift.reset(0, 0);
		else if (triple.count > 1)
			triple.count--;
		else if (triple.count == 1)
			triple.reset(0, 0);

		switch (casting)
		{
		case B1:
			if (element == AF)
			{
				element = Element::NE;
				umbral_hearts = 0;
				enochian = false;
				gauge.reset(0, 0);
				foul_timer.time = 0;
			}
			else
			{
				element = Element::UI;
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
			}
			break;
		case B3:
			element = UI;
			gauge.reset(GAUGE_DURATION, 3);
			push_event(GAUGE_DURATION);
			break;
		case B4:
			umbral_hearts = 3;
			break;
		case F1:
			if (element == UI)
			{
				element = Element::NE;
				umbral_hearts = 0;
				enochian = false;
				gauge.reset(0, 0);
				foul_timer.time = 0;
			}
			else
			{
				element = Element::AF;
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
				if (umbral_hearts > 0)
					umbral_hearts--;
				if (sharp.count > 0 || prob(rng) < FS_PROC_RATE)
				{
					fs_proc.reset(FS_DURATION, 1);
					sharp.reset(0, 0);
					push_event(FS_DURATION);
				}
			}
			break;
		case F3:
			if (fs_proc.count > 0)
				fs_proc.reset(0, 0);
			else if (element == AF && umbral_hearts > 0)
				umbral_hearts--;
			element = AF;
			gauge.reset(GAUGE_DURATION, 3);
			push_event(GAUGE_DURATION);
			break;
		case F4:
			if (umbral_hearts > 0)
				umbral_hearts--;
			break;
		case T3:
			if (tc_proc.count > 0)
				tc_proc.reset(0, 0);
			dot.reset(DOT_DURATION, enochian ? 1 : 2);
			push_event(DOT_DURATION);
			if (sharp.count > 0)
			{
				tc_proc.reset(TC_DURATION, 1);
				sharp.reset(0, 0);
				push_event(TC_DURATION);
			}
			dot_index = history.size() - 1;
			break;
		case FOUL:
			foul_timer.ready = false;
		}
		casting = NONE;
		cast_timer.ready = false;
	}

	int BlackMage::get_mp_cost(int action) const
	{
		switch (action)
		{
		case B1:
			if (element == AF && get_cast_time(B1) < gauge.time)
				return gauge.count == 1 ? B1_MP_COST / 2 : B1_MP_COST / 4;
			return B1_MP_COST;
		case B3:
			if (element == AF && get_cast_time(B3) < gauge.time)
				return gauge.count == 1 ? B3_MP_COST / 2 : B3_MP_COST / 4;
			return B3_MP_COST;
		case B4:
			return B4_MP_COST;
		case F1:
			if (element == UI && get_cast_time(F1) < gauge.time)
				return gauge.count == 1 ? F1_MP_COST / 2 : F1_MP_COST / 4;
			return (element == AF && umbral_hearts == 0) ? F1_MP_COST * 2 : F1_MP_COST;
		case F3:
			if (fs_proc.count > 0)
				return 0;
			if (element == UI && get_cast_time(F3) < gauge.time)
				return gauge.count == 1 ? F3_MP_COST / 2 : F3_MP_COST / 4;
			return (element == AF && umbral_hearts == 0) ? F3_MP_COST * 2 : F3_MP_COST;
		case F4:
			return umbral_hearts == 0 ? F4_MP_COST * 2 : F4_MP_COST;
		case T3:
			return tc_proc.count > 0 ? 0 : T3_MP_COST;
		case FOUL:
			return 0;
		}
		return 99999;
	}

	int BlackMage::get_damage(int action) const
	{
		float potency = 0.0f;
		switch (action)
		{
		case B1:
			if (element == AF)
			{
				if (gauge.count == 1)
					potency = B1_POTENCY * AF1UI1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = B1_POTENCY * AF2UI2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = B1_POTENCY * AF3UI3_MULTIPLIER;
			}
			else
				potency = B1_POTENCY;
			break;
		case B3:
			if (element == AF)
			{
				if (gauge.count == 1)
					potency = B3_POTENCY * AF1UI1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = B3_POTENCY * AF2UI2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = B3_POTENCY * AF3UI3_MULTIPLIER;
			}
			else
				potency = B3_POTENCY;
			break;
		case B4:
			potency = B4_POTENCY;
			break;
		case F1:
			if (element == UI)
			{
				if (gauge.count == 1)
					potency = F1_POTENCY * AF1UI1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = F1_POTENCY * AF2UI2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = F1_POTENCY * AF3UI3_MULTIPLIER;
			}
			else if (element == AF)
			{
				if (gauge.count == 1)
					potency = F1_POTENCY * AF1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = F1_POTENCY * AF2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = F1_POTENCY * AF3_MULTIPLIER;
			}
			else
				potency = F1_POTENCY;
			break;
		case F3:
			if (element == UI)
			{
				if (gauge.count == 1)
					potency = F3_POTENCY * AF1UI1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = F3_POTENCY * AF2UI2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = F3_POTENCY * AF3UI3_MULTIPLIER;
			}
			else if (element == AF)
			{
				if (gauge.count == 1)
					potency = F3_POTENCY * AF1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = F3_POTENCY * AF2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = F3_POTENCY * AF3_MULTIPLIER;
			}
			else
				potency = F3_POTENCY;
			break;
		case F4:
			if (element == AF)
			{
				if (gauge.count == 1)
					potency = F4_POTENCY * AF1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = F4_POTENCY * AF2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = F4_POTENCY * AF3_MULTIPLIER;
			}
			break;
		case T3:
			potency = tc_proc.count > 0 ? TC_POTENCY : T3_POTENCY;
			break;
		case FOUL:
			potency = FOUL_POTENCY;
			break;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		int damage = potency * stats.potency_multiplier * (enochian ? ENO_MULTIPLIER : 1.0) * MAGICK_AND_MEND_MULTIPLIER;
		//if (!training)
		//{
		//	if (prob(rng) < stats.crit_rate) // is_crit
		//		damage *= stats.crit_multiplier;
		//	if (prob(rng) < stats.dhit_rate) // is_dhit
		//		damage *= 1.25;
		//	damage *= damage_range(rng);
		//}
		//else
		//	damage *= stats.expected_multiplier;
		//return damage;
		return damage * stats.expected_multiplier;
	}
	
	int BlackMage::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		int damage = T3_DOT_POTENCY * stats.potency_multiplier * (dot.count == 1 ? ENO_MULTIPLIER : 1.0) * MAGICK_AND_MEND_MULTIPLIER;
		damage *= stats.dot_multiplier;
		//if (!training)
		//{
		//	damage *= damage_range(rng);
		//	if (prob(rng) < stats.crit_rate) // is_crit
		//		damage *= stats.crit_multiplier;
		//	if (prob(rng) < stats.dhit_rate) // is_dhit
		//		damage *= 1.25;
		//}
		//else
		//	damage *= stats.expected_multiplier;
		//return damage;
		return damage * stats.expected_multiplier;
	}

	void BlackMage::get_state(State& state)
	{
		state.data[0] = mp / (float)MAX_MP;
		state.data[1] = element == UI;
		state.data[2] = element == AF;
		state.data[3] = umbral_hearts > 0;
		state.data[4] = enochian;
		state.data[5] = (TICK_TIMER - mp_timer.time) / (float)TICK_TIMER;
		state.data[6] = (TICK_TIMER - dot_timer.time) / (float)TICK_TIMER;
		state.data[7] = gauge.count > 0;
		state.data[8] = gauge.count == 1;
		state.data[9] = gauge.count == 2;
		state.data[10] = gauge.count == 3;
		state.data[11] = gauge.time / (float)GAUGE_DURATION;
		state.data[12] = foul_timer.ready;
		state.data[13] = (FOUL_TIMER - foul_timer.time) / (float)FOUL_TIMER;
		state.data[14] = swift.count > 0;
		state.data[15] = swift.time / (float)SWIFT_DURATION;
		state.data[16] = sharp.count > 0;
		state.data[17] = sharp.time / (float)SHARP_DURATION;
		state.data[18] = triple.count / 3.0f;
		state.data[19] = triple.time / (float)TRIPLE_DURATION;
		state.data[20] = leylines.count > 0;
		state.data[21] = leylines.time / (float)LL_DURATION;
		state.data[22] = fs_proc.count > 0;
		state.data[23] = fs_proc.time / (float)FS_DURATION;
		state.data[24] = tc_proc.count > 0;
		state.data[25] = tc_proc.time / (float)TC_DURATION;
		state.data[26] = dot.count > 0;
		state.data[27] = dot.time / (float)DOT_DURATION;
		state.data[28] = dot.count == 1;
		state.data[29] = swift_cd.ready;
		state.data[30] = swift_cd.time / (float)SWIFT_CD;
		state.data[31] = triple_cd.ready;
		state.data[32] = triple_cd.time / (float)TRIPLE_CD;
		state.data[33] = sharp_cd.ready;
		state.data[34] = sharp_cd.time / (float)SHARP_CD;
		state.data[35] = leylines_cd.ready;
		state.data[36] = leylines_cd.time / (float)LL_CD;
		state.data[37] = convert_cd.ready;
		state.data[38] = convert_cd.time / (float)CONVERT_CD;
		state.data[39] = eno_cd.ready;
		state.data[40] = eno_cd.time / (float)ENO_CD;
		state.data[41] = gcd_timer.ready;
		state.data[42] = gcd_timer.time / 250.0f;
		state.data[43] = umbral_hearts == 1;
		state.data[44] = umbral_hearts == 2;
		state.data[45] = umbral_hearts == 3;
	}

	void* BlackMage::get_history()
	{
		return &history;
	}

	void* BlackMage::get_state()
	{
		return &history.back().t0;
	}
}