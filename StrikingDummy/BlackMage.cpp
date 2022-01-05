#include "BlackMage.h"
#include <assert.h>
#include <algorithm>
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
	BlackMage::BlackMage(Stats& stats, Opener opener, ActionSet action_set) :
		Job(stats, BLM_ATTR),
		opener(opener),
		action_set(action_set),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD))) * 10),
		iii_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * III_GCD))) * 10),
		iv_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * IV_GCD))) * 10),
		despair_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * DESPAIR_GCD))) * 10),
		fast_base_gcd(lround(floor(0.1f * floor(0.5f * floor(this->stats.ss_multiplier * BASE_GCD)))) * 10),
		fast_iii_gcd(lround(floor(0.1f * floor(0.5f * floor(this->stats.ss_multiplier * III_GCD)))) * 10),
		ll_base_gcd(lround(floor(0.1f * floor(0.85 * floor(this->stats.ss_multiplier * BASE_GCD)))) * 10),
		ll_iii_gcd(lround(floor(0.1f * floor(0.85 * floor(this->stats.ss_multiplier * III_GCD)))) * 10),
		ll_iv_gcd(lround(floor(0.1f * floor(0.85 * floor(this->stats.ss_multiplier * IV_GCD)))) * 10),
		ll_despair_gcd(lround(floor(0.1f * floor(0.85 * floor(this->stats.ss_multiplier * DESPAIR_GCD)))) * 10),
		ll_fast_base_gcd(lround(floor(0.1f * floor(0.5f * floor(0.85 * floor(this->stats.ss_multiplier * BASE_GCD))))) * 10),
		ll_fast_iii_gcd(lround(floor(0.1f * floor(0.5f * floor(0.85 * floor(this->stats.ss_multiplier * III_GCD))))) * 10)
	{
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void BlackMage::reset()
	{
		reset(0, 0, 0);
	}

	void BlackMage::reset(int mp_tick, int lucid_tick, int dot_tick)
	{
		timeline = {};

		if (opener == Opener::PRE_B3 || opener == Opener::PRE_LL_B3)
		{
			mp = MAX_MP - B3_MP_COST;
			element = Element::UI;
		}
		else if (opener == Opener::PRE_F3 || opener == Opener::PRE_LL_F3)
		{
			mp = MAX_MP - F3_MP_COST;
			element = Element::AF;
		}
		else if (opener == Opener::PRE_T3)
		{
			mp = MAX_MP - T3_MP_COST;
			element = Element::NE;
		}
		else
		{
			mp = MAX_MP;
			element = Element::NE;
		}

		umbral_hearts = 0;
		enochian = false;
		paradox = false;
		t3p = false;

		// server ticks
		mp_timer.reset(mp_tick <= 0 ? tick(rng) : mp_tick, false);
		dot_timer.reset(dot_tick <= 0 ? tick(rng) : dot_tick, false);
		lucid_timer.reset(lucid_tick <= 0 ? tick(rng) : lucid_tick, false);
		timeline.push_event(mp_timer.time);
		timeline.push_event(dot_timer.time);
		timeline.push_event(lucid_timer.time);
		mp_wait = 0;

		skip_lucid_tick = false;
		skip_transpose_tick = false;

		// misc timers
		gauge.reset(0, 0);
		xeno_timer.reset(0, false);
		sharp_timer.reset(0, false);
		triple_timer.reset(0, false);
		dot_travel_timer.reset(0, false);

		xeno_procs = 0;
		sharp_procs = 2;
		triple_procs = 2;
		dot_travel = 0;

		// buffs
		swift.reset(0, 0);
		sharp.reset(0, 0);
		triple.reset(0, 0);
		leylines.reset(0, 0);
		fs_proc.reset(0, 0);
		tc_proc.reset(0, 0);
		dot.reset(0, 0);
		lucid.reset(0, 0);
		pot.reset(0, 0);

		// cooldowns		
		swift_cd.reset(0, true);
		leylines_cd.reset(0, true);
		manafont_cd.reset(0, true);
		transpose_cd.reset(0, true);
		lucid_cd.reset(0, true);
		pot_cd.reset(0, true);
		amplifier_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = Action::NONE;

		// precast
		if (opener == Opener::PRE_F3 || opener == Opener::PRE_B3 || opener == Opener::PRE_LL_F3 || opener == Opener::PRE_LL_B3)
		{
			sharp_procs = 1;
			sharp.reset(SHARP_DURATION - 15000, 1);
			sharp_timer.reset(SHARP_CD - 15000, false);
			enochian = true;
			gauge.reset(GAUGE_DURATION - CAST_LOCK - ACTION_TAX, 3);
			xeno_timer.reset(XENO_TIMER, false);
			timeline.push_event(gauge.time);
			timeline.push_event(sharp.time);
			timeline.push_event(sharp_timer.time);
			timeline.push_event(xeno_timer.time);
			sharp_last = -15000;
		}
		if (opener == Opener::PRE_LL_B3 || opener == Opener::PRE_LL_F3)
		{
			leylines.reset(LL_DURATION - 4000, 1);
			leylines_cd.reset(LL_CD - 4000, false);
			timeline.push_event(leylines.time);
			timeline.push_event(leylines_cd.time);
			ll_last = -4000;
		}
		if (opener == Opener::PRE_T3)
		{
			dot_travel = 1;
			dot_travel_timer.reset(DOT_TRAVEL_DURATION - CAST_LOCK - ACTION_TAX, false);
			tc_proc.reset(TC_DURATION - CAST_LOCK - ACTION_TAX, 1);
			sharp_procs = 1;
			sharp_timer.reset(SHARP_CD - 15000, false);
			timeline.push_event(dot_travel_timer.time);
			timeline.push_event(tc_proc.time);
			timeline.push_event(sharp_timer.time);
		}

		// metrics
		xeno_count = 0;
		f1_count = 0;
		f4_count = 0;
		b1_count = 0;
		b3_count = 0;
		b4_count = 0;
		t3_count = 0;
		despair_count = 0;
		transpose_count = 0;
		lucid_count = 0;
		pot_count = 0;
		total_dot_time = 0;

		total_f4_damage = 0.0f;
		total_desp_damage = 0.0f;
		total_xeno_damage = 0.0f;
		total_t3_damage = 0.0f;
		total_dot_damage = 0.0f;
		total_damage = 0.0f;

		history.clear();

		update_history();
	}

	void BlackMage::update(int elapsed)
	{
		assert(elapsed > 0);

		// time metrics
		if (dot.time > 0)
			total_dot_time += elapsed;

		if (element != Element::AF && mp != MAX_MP)
			mp_wait += elapsed;

		assert(mp_wait <= TICK_TIMER);

		// server ticks
		mp_timer.update(elapsed);
		dot_timer.update(elapsed);
		lucid_timer.update(elapsed);

		// misc timer
		gauge.update(elapsed);
		xeno_timer.update(elapsed);
		sharp_timer.update(elapsed);
		triple_timer.update(elapsed);
		dot_travel_timer.update(elapsed);

		// buffs
		swift.update(elapsed);
		sharp.update(elapsed);
		triple.update(elapsed);
		leylines.update(elapsed);
		fs_proc.update(elapsed);
		tc_proc.update(elapsed);
		dot.update(elapsed);
		lucid.update(elapsed);
		pot.update(elapsed);

		// cooldowns
		swift_cd.update(elapsed);
		leylines_cd.update(elapsed);
		manafont_cd.update(elapsed);
		transpose_cd.update(elapsed);
		lucid_cd.update(elapsed);
		pot_cd.update(elapsed);
		amplifier_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		//
		if (dot_travel_timer.ready)
		{
			dot.reset(DOT_DURATION, dot_travel);
			dot_travel_timer.ready = false;
			dot_travel = 0;
			push_event(DOT_DURATION);
		}
		if (mp_timer.ready)
			update_mp();
		if (dot_timer.ready)
			update_dot();
		if (lucid_timer.ready)
			update_lucid();
		if (element != Element::NE && gauge.count == 0)
		{
			element = Element::NE;
			umbral_hearts = 0;
			enochian = false;
			paradox = false;
			xeno_timer.time = 0;
		}
		if (enochian && xeno_timer.time == 0)
		{
			xeno_timer.time = XENO_TIMER;
			assert(xeno_timer.ready);
			xeno_procs = std::min(xeno_procs + 1, 2);
			xeno_timer.ready = false;
			push_event(XENO_TIMER);
		}
		if (sharp_timer.ready)
		{
			sharp_procs++;
			assert(sharp_timer.time == 0);
			assert(sharp_procs <= 2);
			sharp_timer.ready = false;
			if (sharp_procs < 2)
			{
				sharp_timer.time = SHARP_CD;
				push_event(SHARP_CD);
			}
		}
		if (triple_timer.ready)
		{
			triple_procs++;
			assert(triple_timer.time == 0);
			assert(triple_procs <= 2);
			triple_timer.ready = false;
			if (triple_procs < 2)
			{
				triple_timer.time = TRIPLE_CD;
				push_event(TRIPLE_CD);
			}
		}
		if (cast_timer.ready)
			end_action();
		if (element == Element::AF || mp == MAX_MP)
			mp_wait = 0;

		update_history();
	}

	void BlackMage::update_history()
	{
		actions.clear();
		if (action_timer.ready)
		{
			for (int i = 0; i < NUM_ACTIONS; i++)
				if (can_use_action(i))
					actions.push_back(i);

			if (actions.empty() || (actions.size() == 1 && actions[0] == NONE))
				return;

			// state/transition
			if (!history.empty())
			{
				Transition& t = history.back();
				get_state(t.t1);
				t.dt = timeline.time - t.time;
				t.actions = actions;
			}

			history.emplace_back();
			Transition& t = history.back();
			get_state(t.t0);
			t.reward = 0.0f;
			t.time = timeline.time;
		}
	}

	void BlackMage::update_mp()
	{
		if (!skip_transpose_tick)
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
		}
		mp_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
		mp_wait = 0;
		skip_transpose_tick = false;
	}

	void BlackMage::update_dot()
	{
		if (dot.count > 0)
		{
			float damage = get_dot_damage();
			total_damage += damage;
			total_dot_damage += damage;
			history.back().reward += damage;
			if (prob(rng) < TC_PROC_RATE)
			{
				tc_proc.reset(TC_REFRESH_DURATION, 1);
				push_event(TC_REFRESH_DURATION);
			}
		}
		dot_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void BlackMage::update_lucid()
	{
		if (lucid.count > 0)
		{
			if (!skip_lucid_tick)
			{
				if (element != Element::AF)
				{
					mp += LUCID_MP;
					if (mp > MAX_MP)
						mp = MAX_MP;
				}
			}
			skip_lucid_tick = false;
		}
		lucid_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void BlackMage::update_metric(int action, float damage)
	{
		switch (action)
		{
		case XENO:
			xeno_count++;
			total_xeno_damage += damage;
			break;
		case F1:
			f1_count++;
			break;
		case F4:
			f4_count++;
			total_f4_damage += damage;
			break;
		case B1:
			b1_count++;
			break;
		case B3:
			b3_count++;
			break;
		case B4:
			b4_count++;
			break;
		case T3:
			t3_count++;
			total_t3_damage += damage;
			break;
		case DESPAIR:
			despair_count++;
			total_desp_damage += damage;
			break;
		case TRANSPOSE:
			transpose_count++;
			break;
		case LUCID:
			lucid_count++;
			break;
		case POT:
			pot_count++;
			break;
		}
		// distribution metrics
		if (!dist_metrics_enabled)
			return;
		switch (action)
		{
		case T3:
			if (tc_proc.count > 0)
				t3p_dist.push_back(timeline.time - t3_last + DOT_DURATION);
			else
				t3_dist.push_back(timeline.time - t3_last + DOT_DURATION);
			t3_last = timeline.time;
			break;
		case SWIFT:
			swift_dist.push_back(timeline.time - swift_last + SWIFT_CD);
			swift_last = timeline.time;
			break;
		case TRIPLE:
			triple_dist.push_back(timeline.time - triple_last + TRIPLE_CD);
			triple_last = timeline.time;
			break;
		case SHARP:
			sharp_dist.push_back(timeline.time - sharp_last + SHARP_CD);
			sharp_last = timeline.time;
			break;
		case LEYLINES:
			ll_dist.push_back(timeline.time - ll_last + LL_CD);
			ll_last = timeline.time;
			break;
		case MANAFONT:
			mf_dist.push_back(timeline.time - mf_last + MANAFONT_CD);
			mf_last = timeline.time;
			break;
		}
	}

	bool BlackMage::is_instant_cast(int action) const
	{
		// for gcds
		return swift.count == 1 || triple.count > 0 || (action == F3 && fs_proc.count > 0) || (action == T3 && tc_proc.count > 0) || action == XENO || (action == PARADOX && element == UI);
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
				return get_ll_cast_time(ll_fast_base_gcd, fast_base_gcd) - CAST_LOCK;
			return get_ll_cast_time(ll_base_gcd, base_gcd) - CAST_LOCK;
		case B3:
			if (element == AF && gauge.count == 3)
				return get_ll_cast_time(ll_fast_iii_gcd, fast_iii_gcd) - CAST_LOCK;
			return get_ll_cast_time(ll_iii_gcd, iii_gcd) - CAST_LOCK;
		case F1:
			if (element == UI && gauge.count == 3)
				return get_ll_cast_time(ll_fast_base_gcd, fast_base_gcd) - CAST_LOCK;
			return get_ll_cast_time(ll_base_gcd, base_gcd) - CAST_LOCK;
		case F3:
			if (element == UI && gauge.count == 3)
				return get_ll_cast_time(ll_fast_iii_gcd, fast_iii_gcd) - CAST_LOCK;
			return get_ll_cast_time(ll_iii_gcd, iii_gcd) - CAST_LOCK;
		case F4:
			return get_ll_cast_time(ll_iv_gcd, iv_gcd) - CAST_LOCK;
		case B4:
		case T3:
		case XENO:
		case PARADOX:
			return get_ll_cast_time(ll_base_gcd, base_gcd) - CAST_LOCK;
		case DESPAIR:
			return get_ll_cast_time(ll_despair_gcd, despair_gcd) - CAST_LOCK;
		}
		return 99999;
	}

	int BlackMage::get_action_time(int action) const
	{
		if (is_instant_cast(action))
			return ANIMATION_LOCK + ACTION_TAX;
		return get_cast_time(action) + CAST_LOCK + ACTION_TAX;
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
			return !gcd_timer.ready;
		case B1:
			return gcd_timer.ready && !paradox && get_mp_cost(B1) <= mp;
		case B3:
			return gcd_timer.ready && get_mp_cost(B3) <= mp;
		case B4:
			return gcd_timer.ready && element == UI && enochian && get_cast_time(B4) < gauge.time && get_mp_cost(B4) <= mp;
		case F1:
			return gcd_timer.ready && !paradox && get_mp_cost(F1) <= mp;
		case F3:
			if (action_set == ActionSet::STANDARD)
				return gcd_timer.ready && get_mp_cost(F3) <= mp && (element != UI || umbral_hearts == 3);
			else
				return gcd_timer.ready && get_mp_cost(F3) <= mp;
		case F4:
			return gcd_timer.ready && element == AF && enochian && get_cast_time(F4) < gauge.time && get_mp_cost(F4) <= mp;
		case T3:
			return gcd_timer.ready && get_mp_cost(T3) <= mp;
		case XENO:
			return gcd_timer.ready && xeno_procs > 0;
		case DESPAIR:
			return gcd_timer.ready && element == AF && enochian && get_cast_time(DESPAIR) < gauge.time && get_mp_cost(DESPAIR) <= mp;
		case PARADOX:
			return gcd_timer.ready && paradox && get_mp_cost(PARADOX) <= mp;
		case SWIFT:
			return swift_cd.ready;
		case TRIPLE:
			return triple_procs > 0;
		case SHARP:
			return sharp_procs > 0;
		case LEYLINES:
			return leylines_cd.ready;
		case MANAFONT:
			return manafont_cd.ready && mp < MAX_MP;
		case TRANSPOSE:
			if (action_set == ActionSet::FULL)
				return transpose_cd.ready && element != Element::NE;
			else
				return false;
		case AMPLIFIER:
			return amplifier_cd.ready && element != Element::NE && xeno_procs < 2;
		case LUCID:
			if (action_set == ActionSet::STANDARD)
				return false;
			else
				return lucid_cd.ready;
		case WAIT_FOR_MP:
			if (action_set == ActionSet::FULL)
				return gcd_timer.ready && element != Element::AF && mp < MAX_MP;
			else
				return false;
		case POT:
			return pot_cd.ready;
		case F3P_OFF:
			if (action_set == ActionSet::FULL)
				return fs_proc.count > 0;
			else
				return false;
		}
		return false;
	}

	void BlackMage::use_action(int action)
	{
		int mp_time;
		history.back().action = action;
		switch (action)
		{
		case NONE:
			return;
		case WAIT_FOR_MP:
			mp_time = mp_timer.time;
			if (lucid.count > 0)
			{
				int lucid_time = lucid_timer.time + (skip_lucid_tick ? TICK_TIMER : 0);
				if (lucid_time < lucid.time && lucid_time < mp_time)
					mp_time = lucid_time;
			}
			action_timer.reset(mp_time, false);
			push_event(action_timer.time);
			return;
		case B1:
		case B3:
		case B4:
		case F1:
		case F3:
		case F4:
		case T3:
		case XENO:
		case DESPAIR:
		case PARADOX:
			gcd_timer.reset(get_gcd_time(action), false);
			cast_timer.reset(get_cast_time(action), false);
			action_timer.reset(get_action_time(action), false);
			casting = action;
			if (casting == T3)
				t3p = tc_proc.count > 0;
			if (cast_timer.time == 0)
				end_action();
			push_event(gcd_timer.time);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			return;
		case SWIFT:
			swift.reset(SWIFT_DURATION, 1);
			swift_cd.reset(SWIFT_CD, false);
			push_event(SWIFT_DURATION);
			push_event(SWIFT_CD);
			break;
		case TRIPLE:
			triple_procs--;
			if (triple_timer.time == 0)
			{
				assert(!triple_timer.ready);
				triple_timer.reset(TRIPLE_CD, false);
				push_event(TRIPLE_CD);
			}
			triple.reset(TRIPLE_DURATION, 3);
			push_event(TRIPLE_DURATION);
			break;
		case SHARP:
			sharp_procs--;
			if (sharp_timer.time == 0)
			{
				assert(!sharp_timer.ready);
				sharp_timer.reset(SHARP_CD, false);
				push_event(SHARP_CD);
			}
			sharp.reset(SHARP_DURATION, 1);
			push_event(SHARP_DURATION);
			break;
		case LEYLINES:
			leylines.reset(LL_DURATION, 1);
			leylines_cd.reset(LL_CD, false);
			push_event(LL_DURATION);
			push_event(LL_CD);
			break;
		case MANAFONT:
			mp = std::min(mp + MANAFONT_MP, MAX_MP);
			manafont_cd.reset(MANAFONT_CD, false);
			push_event(MANAFONT_CD);
			break;
		case TRANSPOSE:
			assert(element != Element::NE);
			if (((element == Element::UI && umbral_hearts == 3) || element == Element::AF) && gauge.count == 3)
				paradox = true;
			element = element == Element::AF ? Element::UI : Element::AF;
			skip_transpose_tick = mp_timer.time <= LATENCY;
			transpose_cd.reset(TRANSPOSE_CD, false);
			gauge.reset(GAUGE_DURATION, 1);
			push_event(TRANSPOSE_CD);
			push_event(GAUGE_DURATION);
			break;
		case AMPLIFIER:
			assert(element != Element::NE);
			assert(xeno_procs < 2);
			xeno_procs++;
			amplifier_cd.reset(AMPLIFIER_CD, false);
			push_event(AMPLIFIER_CD);
			break;
		case LUCID:
			skip_lucid_tick = lucid_timer.time <= ANIMATION_LOCK + ACTION_TAX;
			lucid.reset(LUCID_DURATION, 1);
			lucid_cd.reset(LUCID_CD, false);
			push_event(LUCID_DURATION);
			push_event(LUCID_CD);
			break;
		case POT:
			pot.reset(POT_DURATION, 1);
			pot_cd.reset(POT_CD, false);
			push_event(POT_DURATION);
			push_event(POT_CD);
			break;
		case F3P_OFF:
			fs_proc.reset(0, 0);
			action_timer.reset(1, false);
			push_event(action_timer.time);
			return;
		}
		// ogcd only
		action_timer.reset(action == POT ? POTION_LOCK + ACTION_TAX : ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
		update_metric(action);
	}

	void BlackMage::end_action()
	{
		assert(cast_timer.time == 0);
		assert(cast_timer.ready || is_instant_cast(casting));

		int casting_mp_cost = get_mp_cost(casting, true);

		assert(casting != NONE);
		assert(mp >= casting_mp_cost);
		float damage = get_damage(casting);
		total_damage += damage;
		history.back().reward += damage;
		update_metric(casting, damage);

		if (casting == DESPAIR)
			mp = 0;
		else
			mp -= casting_mp_cost;

		if (casting == F3 && fs_proc.count > 0);
		// firestarter doesn't use swift or triple
		else if (casting == T3 && tc_proc.count > 0);
		// thundercloud doesn't use swift or triple
		else if (casting == XENO);
		// xeno doesn't use swift or triple
		else if (casting == PARADOX && element == UI);
		// paradox in UI doesn't use swift or triple
		else if (swift.count > 0)
			swift.reset(0, 0);
		else if (triple.count > 1)
			triple.count--;
		else if (triple.count == 1)
			triple.reset(0, 0);

		switch (casting)
		{
		case B1:
			if (element == Element::AF)
			{
				element = Element::NE;
				umbral_hearts = 0;
				enochian = false;
				gauge.reset(0, 0);
				xeno_timer.time = 0;
			}
			else
			{
				element = Element::UI;
				if (!enochian)
				{
					enochian = true;
					xeno_timer.time = XENO_TIMER;
					push_event(XENO_TIMER);
				}
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
			}
			break;
		case B3:
			if (element == Element::AF && gauge.count == 3)
				paradox = true;
			element = Element::UI;
			if (!enochian)
			{
				enochian = true;
				xeno_timer.time = XENO_TIMER;
				push_event(XENO_TIMER);
			}
			gauge.reset(GAUGE_DURATION, 3);
			push_event(GAUGE_DURATION);
			break;
		case B4:
			umbral_hearts = 3;
			break;
		case F1:
			if (element == Element::UI)
			{
				element = Element::NE;
				umbral_hearts = 0;
				enochian = false;
				gauge.reset(0, 0);
				xeno_timer.time = 0;
			}
			else
			{
				element = Element::AF;
				if (!enochian)
				{
					enochian = true;
					xeno_timer.time = XENO_TIMER;
					push_event(XENO_TIMER);
				}
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
				if (umbral_hearts > 0)
					umbral_hearts--;
			}
			if (sharp.count > 0 || prob(rng) < FS_PROC_RATE)
			{
				fs_proc.reset(FS_DURATION, 1);
				sharp.reset(0, 0);
				push_event(FS_DURATION);
			}
			break;
		case F3:
			if (fs_proc.count > 0)
				fs_proc.reset(0, 0);
			else if (element == Element::AF && umbral_hearts > 0)
				umbral_hearts--;
			if (element == Element::UI && gauge.count == 3 && umbral_hearts == 3)
				paradox = true;
			element = Element::AF;
			if (!enochian)
			{
				enochian = true;
				xeno_timer.time = XENO_TIMER;
				push_event(XENO_TIMER);
			}
			gauge.reset(GAUGE_DURATION, 3);
			push_event(GAUGE_DURATION);
			break;
		case F4:
			if (umbral_hearts > 0)
				umbral_hearts--;
			break;
		case T3:
			if (t3p)
				tc_proc.reset(0, 0);
			if (dot_travel_timer.time > 0)
			{
				dot.reset(DOT_DURATION + dot_travel_timer.time, dot_travel);
				push_event(DOT_DURATION + dot_travel_timer.time);
			}
			dot_travel_timer.reset(DOT_TRAVEL_DURATION, false);
			dot_travel = (1 | (enochian ? 2 : 0) | (pot.count > 0 ? 4 : 0));
			push_event(DOT_TRAVEL_DURATION);
			if (sharp.count > 0)
			{
				tc_proc.reset(TC_DURATION, 1);
				sharp.reset(0, 0);
				push_event(TC_DURATION);
			}
			break;
		case XENO:
			assert(xeno_procs > 0);
			xeno_procs--;
			break;
		case DESPAIR:
			element = Element::AF;
			gauge.reset(GAUGE_DURATION, 3);
			push_event(GAUGE_DURATION);
			break;
		case PARADOX:
			paradox = false;
			if (element == Element::AF)
			{
				if (sharp.count > 0 || prob(rng) < FS_PROC_RATE)
				{
					fs_proc.reset(FS_DURATION, 1);
					sharp.reset(0, 0);
					push_event(FS_DURATION);
				}
			}
			if (element != Element::NE)
			{
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
			}
			break;
		}
		casting = NONE;
		cast_timer.ready = false;
	}

	int BlackMage::get_mp_cost(int action, bool is_end_action) const
	{
		switch (action)
		{
		case B1:
			if (element == AF && (is_end_action || get_cast_time(B1) < gauge.time))
				return 0;
			else if (element == UI && (is_end_action || get_cast_time(B1) < gauge.time))
			{
				if (gauge.count == 1)
					return B1_MP_COST * 3 / 4;
				else if (gauge.count == 2)
					return B1_MP_COST / 2;
				return 0;
			}
			return B1_MP_COST;
		case B3:
			if (element == AF && (is_end_action || get_cast_time(B3) < gauge.time))
				return 0;
			else if (element == UI && (is_end_action || get_cast_time(B3) < gauge.time))
			{
				if (gauge.count == 1)
					return B3_MP_COST * 3 / 4;
				else if (gauge.count == 2)
					return B3_MP_COST / 2;
				return 0;
			}
			return B3_MP_COST;
		case B4:
			if (element == UI && (is_end_action || get_cast_time(B4) < gauge.time))
			{
				if (gauge.count == 1)
					return B4_MP_COST * 3 / 4;
				else if (gauge.count == 2)
					return B4_MP_COST / 2;
				return 0;
			}
			return B4_MP_COST;
		case F1:
			if (element == UI && (is_end_action || get_cast_time(F1) < gauge.time))
				return 0;
			return (element == AF && umbral_hearts == 0) ? F1_MP_COST * 2 : F1_MP_COST;
		case F3:
			if (fs_proc.count > 0)
				return 0;
			if (element == UI && (is_end_action || get_cast_time(F3) < gauge.time))
				return 0;
			return (element == AF && umbral_hearts == 0) ? F3_MP_COST * 2 : F3_MP_COST;
		case F4:
			return umbral_hearts == 0 ? F4_MP_COST * 2 : F4_MP_COST;
		case T3:
			return (is_end_action && t3p) || (!is_end_action && tc_proc.count > 0) ? 0 : T3_MP_COST;
		case XENO:
			return 0;
		case DESPAIR:
			return DESPAIR_MP_COST;
		case PARADOX:
			return element == Element::UI && (is_end_action || get_cast_time(PARADOX) < gauge.time) ? 0 : PARADOX_MP_COST;
		}
		return 0;
	}

	float BlackMage::get_damage(int action)
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
			potency = t3p ? TC_POTENCY : T3_POTENCY;
			break;
		case XENO:
			potency = XENO_POTENCY;
			break;
		case DESPAIR:
			if (element == AF)
			{
				if (gauge.count == 1)
					potency = DESPAIR_POTENCY * AF1_MULTIPLIER;
				else if (gauge.count == 2)
					potency = DESPAIR_POTENCY * AF2_MULTIPLIER;
				else if (gauge.count == 3)
					potency = DESPAIR_POTENCY * AF3_MULTIPLIER;
			}
			break;
		case PARADOX:
			potency = PARADOX_POTENCY;
			break;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		return potency * stats.potency_multiplier * stats.expected_multiplier * (enochian ? ENO_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
		//float rng_multiplier = damage_range(rng) * (prob(rng) < stats.crit_rate ? stats.crit_multiplier : 1.0f) * (prob(rng) < stats.dhit_rate ? 1.25f : 1.0f);
		//return potency * stats.potency_multiplier * rng_multiplier * (enochian ? ENO_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
	}

	float BlackMage::get_dot_damage()
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return T3_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * stats.expected_multiplier * ((dot.count & 2) ? ENO_MULTIPLIER : 1.0f) * ((dot.count & 4) ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
		//float rng_multiplier = damage_range(rng) * (prob(rng) < stats.crit_rate ? stats.crit_multiplier : 1.0f) * (prob(rng) < stats.dhit_rate ? 1.25f : 1.0f);
		//return T3_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * rng_multiplier * ((dot.count & 2) ? ENO_MULTIPLIER : 1.0f) * ((dot.count & 4) ? stats.pot_multiplier : 1.0f) * MAGICK_AND_MEND_MULTIPLIER;
	}

	void BlackMage::get_state(float* state)
	{
		state[0] = mp / (float)MAX_MP;
		state[1] = element == UI;
		state[2] = element == AF;
		state[3] = umbral_hearts > 0;
		state[4] = umbral_hearts == 1;
		state[5] = umbral_hearts == 2;
		state[6] = umbral_hearts == 3;
		state[7] = enochian;
		state[8] = paradox;
		state[9] = gauge.count == 1;
		state[10] = gauge.count == 2;
		state[11] = gauge.count == 3;
		state[12] = gauge.time / (float)GAUGE_DURATION;
		state[13] = xeno_procs > 0;
		state[14] = xeno_procs > 1;
		state[15] = (XENO_TIMER - xeno_timer.time) / (float)XENO_TIMER;
		state[16] = swift.count > 0;
		state[17] = swift.time / (float)SWIFT_DURATION;
		state[18] = sharp.count > 0;
		state[19] = sharp.time / (float)SHARP_DURATION;
		state[20] = triple.count / 3.0f;
		state[21] = triple.time / (float)TRIPLE_DURATION;
		state[22] = leylines.count > 0;
		state[23] = leylines.time / (float)LL_DURATION;
		state[24] = fs_proc.count > 0;
		state[25] = fs_proc.time / (float)FS_DURATION;
		state[26] = tc_proc.count > 0;
		state[27] = tc_proc.time / (float)TC_DURATION;
		state[28] = dot.count > 0;
		state[29] = dot.time / (float)DOT_DURATION;
		state[30] = (dot.count & 2) != 0;
		state[31] = (dot.count & 4) != 0;
		state[32] = lucid.count > 0;
		state[33] = lucid.time / (float)LUCID_DURATION;
		state[34] = swift_cd.ready;
		state[35] = swift_cd.time / (float)SWIFT_CD;
		state[36] = triple_procs > 0;
		state[37] = triple_procs > 1;
		state[38] = (TRIPLE_CD - triple_timer.time) / (float)TRIPLE_CD;
		state[39] = sharp_procs > 0;
		state[40] = sharp_procs > 1;
		state[41] = (SHARP_CD - sharp_timer.time) / (float)SHARP_CD;
		state[42] = leylines_cd.ready;
		state[43] = leylines_cd.time / (float)LL_CD;
		state[44] = manafont_cd.ready;
		state[45] = manafont_cd.time / (float)MANAFONT_CD;
		state[46] = lucid_cd.ready;
		state[47] = lucid_cd.time / (float)LUCID_CD;
		state[48] = gcd_timer.ready;
		state[49] = gcd_timer.time / (BASE_GCD * 1000.0f);
		state[50] = transpose_cd.ready;
		state[51] = transpose_cd.time / (float)TRANSPOSE_CD;
		state[52] = pot.count > 0;
		state[53] = pot.time / (float)POT_DURATION;
		state[54] = pot_cd.ready;
		state[55] = pot_cd.time / (float)POT_CD;
		state[56] = mp_wait / (float)TICK_TIMER;
		state[57] = 0.0f;
		//state[56] = mp_timer.time / (float)TICK_TIMER;
		//state[57] = lucid_timer.time / (float)TICK_TIMER;
		state[58] = dot_travel > 0;
		state[59] = dot_travel_timer.time / (float)DOT_TRAVEL_DURATION;
		state[60] = (dot_travel & 2) != 0;
		state[61] = (dot_travel & 4) != 0;
		state[62] = amplifier_cd.ready;
		state[63] = amplifier_cd.time / (float)AMPLIFIER_CD;
	}

	std::string BlackMage::get_info()
	{
		return "\n";
	}
}