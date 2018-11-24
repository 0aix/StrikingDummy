#include "StrikingDummy.h"
#include "Logger.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <iostream>

namespace StrikingDummy
{
	std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::uniform_real_distribution<double> prob(0.0, 1.0);
	std::uniform_real_distribution<double> damage_range(0.95, 1.05);
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

		swift_cd.ready = true;
		triple_cd.ready = true;
		sharp_cd.ready = true;
		leylines_cd.ready = true;
		convert_cd.ready = true;
		eno_cd.ready = true;

		gcd_timer.ready = true;
		lock_timer.ready = true;
	}

	void BlackMage::start(Rotation* rotation, int time_limit_in_seconds)
	{
		rotation->job = this;
		time_limit_in_seconds *= 100; // seconds -> centiseconds

		// Precast
		int precast_time = get_cast_time(BlackMage::B3);

		// Set up mp and dot timers
		std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		std::uniform_int_distribution<int> tick_rng(1, 300);

		mp_timer.time = tick_rng(rng);
		dot_timer.time = tick_rng(rng);

		timeline.time = -precast_time;
		timeline.push_event(mp_timer.time);
		timeline.push_event(dot_timer.time);
		timeline.push_event(0);

		long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		int elapsed;
		while ((elapsed = timeline.next_event()) >= 0)
		{
			update(elapsed);
			if (timeline.time > time_limit_in_seconds)
				break;
			std::remove_copy_if(int_range.cbegin(), int_range.cend(), std::back_inserter(actions), [this](int i) { return !this->can_use_action((Action)i); });
			use_action((Action)rotation->choose_action(actions));
			actions.clear();
		}

		long long end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		std::cout << "total time: " << timeline.time / 100.0 << " seconds" << std::endl;
		std::cout << "ending dps: " << total_damage / (timeline.time / 100.0) << std::endl;
		std::cout << "running time: " << (end_time - start_time) / 1000000000.0 << " seconds" << std::endl;
	}

	void BlackMage::train()
	{
		training = true;
	}

	void BlackMage::update(int elapsed)
	{
		damage_reward = 0;

		if (elapsed == 0)
			return;

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
		eno_cd.update(elapsed);
		swift_cd.update(elapsed);
		triple_cd.update(elapsed);
		sharp_cd.update(elapsed);
		leylines_cd.update(elapsed);
		convert_cd.update(elapsed);

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

		total_damage += damage_reward;
	}

	void BlackMage::step()
	{
		damage_reward = 0;

		// server ticks
		mp_timer.step();
		dot_timer.step();

		// elemental gauge
		gauge.step();
		foul_timer.step();

		// buff timers
		swift.step();
		sharp.step();
		triple.step();
		leylines.step();
		fs_proc.step();
		tc_proc.step();
		dot.step();

		// cooldown timers
		eno_cd.step();
		swift_cd.step();
		triple_cd.step();
		sharp_cd.step();
		leylines_cd.step();
		convert_cd.step();

		// action timers
		gcd_timer.step();
		cast_timer.step();
		lock_timer.step();

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

		total_damage += damage_reward;
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
			damage_reward += get_dot_damage();
			if (prob(rng) < 0.10)
			{
				tc_proc.reset(TC_DURATION, 1);
				push_event(TC_DURATION);
			}
		}
		dot_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	bool BlackMage::is_instant_cast(Action action) const
	{
		// for gcds
		return swift.count  == 1|| triple.count > 0 || (action == F3 && fs_proc.count > 0) || (action == T3 && tc_proc.count > 0);
	}

	int BlackMage::get_ll_cast_time(int ll_cast_time, int cast_time) const
	{
		return (leylines.count && ll_cast_time < leylines.time) ? ll_cast_time : cast_time;
	}

	int BlackMage::get_cast_time(Action action) const
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

	int BlackMage::get_lock_time(Action action) const
	{
		if (is_instant_cast(action))
			return ANIMATION_LOCK + CASTER_TAX;
		return get_cast_time(action) + CASTER_TAX;
	}

	int BlackMage::get_gcd_time(Action action) const
	{
		if (is_instant_cast(action))
			return leylines.count > 0 ? ll_base_gcd : base_gcd;
		return get_ll_cast_time(ll_base_gcd, base_gcd);
	}

	bool BlackMage::can_use_action(Action action) const
	{
		if (action == Action::NONE)
			return true;
		if (!lock_timer.ready)
			return false;
		switch (action)
		{
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
			return eno_cd.ready && element != Element::NE;
		}
		return false;
	}

	void BlackMage::use_action(Action action)
	{
		if (action != NONE)
			DBG(std::cout << timeline.time / 100.0 << ": " << action_names[action].c_str() << std::endl);

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
			gcd_timer.reset(get_gcd_time(action), false);
			cast_timer.reset(get_cast_time(action), false);
			lock_timer.reset(get_lock_time(action), false);
			casting = action;
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

		mp -= get_mp_cost(casting);

		assert(mp >= 0);

		damage_reward += get_damage(casting);

		if (casting == F3 && fs_proc.count > 0);
			// firestarter doesn't deplete swift or triple
		else if (casting == T3 && tc_proc.count > 0);
			// thundercloud doesn't deplete swift or triple
		else if (swift.count > 0)
			swift.reset(0, 0);
		else if (triple.count > 0)
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
			}
			else
			{
				element = Element::AF;
				gauge.reset(GAUGE_DURATION, std::min(gauge.count + 1, 3));
				push_event(GAUGE_DURATION);
				if (umbral_hearts > 0)
					umbral_hearts--;
				if (sharp.count > 0 || prob(rng) < 0.40)
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
			break;
		case FOUL:
			foul_timer.ready = false;
		}

		casting = NONE;
		cast_timer.ready = false;
	}

	int BlackMage::get_mp_cost(Action action) const
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

	int BlackMage::get_damage(Action action) const
	{
		double potency = 0.0;
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
		int damage = potency / 100.0 * stats.wep_multiplier * stats.attk_multiplier * stats.det_multiplier * (enochian ? ENO_MULTIPLIER : 1.0) * MAGICK_AND_MEND_MULTIPLIER;
		if (prob(rng) < stats.crit_rate) // is_crit
			damage *= stats.crit_multiplier;
		if (prob(rng) < stats.dhit_rate) // is_dhit
			damage *= 1.25;
		damage *= damage_range(rng);
		return damage;
	}
	
	int BlackMage::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		int damage = T3_DOT_POTENCY / 100.0 * stats.wep_multiplier * stats.attk_multiplier * stats.det_multiplier * (dot.count == 1 ? ENO_MULTIPLIER : 1.0) * MAGICK_AND_MEND_MULTIPLIER;
		damage *= stats.dot_multiplier;
		damage *= damage_range(rng);
		if (prob(rng) < stats.crit_rate) // is_crit
			damage *= stats.crit_multiplier;
		if (prob(rng) < stats.dhit_rate) // is_dhit
			damage *= 1.25;
		return damage;
	}
}