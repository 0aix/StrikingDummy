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
	BlackMage::BlackMage(Stats& stats, Opener opener) : Job(stats), opener(opener)
	{
		actions.reserve(NUM_ACTIONS);
		reset();

		crit_rate = 0.05f + stats.crit / (stats.crit + STAT_MOD);
		luck_rate = 0.05f + stats.luck / (stats.luck + STAT_MOD);
		crit_multi = 0.50f + stats.base_crit_multi;
		luck_multi = (0.40f + 0.25f * luck_rate) * LUCKY_STRIKE_MULTIPLIER;
	}

	void BlackMage::reset()
	{
		reset(0);
	}

	void BlackMage::reset(int gauge_tick)
	{
		timeline = {};

		gauge = MAX_GAUGE;
		sharp = 0;
		impact = 0;

		galeform_active = false;
		tempestrike_gauge = 0.0f;
		falcon_gauge = 0.0f;

		in_air = false;
		azure = false;
		prev_falcon_toss = false;
		enhanced_galeform_next = false;

		// gauge
		gauge_timer.reset(gauge_tick <= 0 ? tick(rng) : gauge_tick, false);
		timeline.push_event(gauge_timer.time);

		galeform_gauge_timer.reset(0, false);
		inspire_gauge_timer.reset(0, false);
		inspire_sharp_timer.reset(0, false);

		// misc timers
		galeform_timer.reset(0, false);
		sharp_timer.reset(0, false);
		tornado_timer_1.reset(0, false);
		tornado_timer_2.reset(0, false);
		tornado_timer_3.reset(0, false);

		galeform_procs = 0;

		// buffs
		chasing_step.reset(0, 0);
		inspire.reset(0, 0);
		typhoon_cleave.reset(0, 0);
		windfury.reset(0, 0);
		galeform.reset(0, 0);
		tempestrike.reset(0, 0);
		divine_haste.reset(0, 0);
		chasing_str.reset(0, 0);
		set_bonus_dmg.reset(0, 0);
		tornado_1.reset(0, 0);
		tornado_2.reset(0, 0);
		tornado_3.reset(0, 0);

		// cooldowns
		typhoon_cleave_cd.reset(0, true);
		falcon_toss_cd.reset(0, true);
		spear_thrust_cd.reset(0, true);

		// actions
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = -1;
		cast_frame = -1;
		cast_speed = 1.0f;

		if (opener == Opener::GAUGE)
		{
			sharp = 6;
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(SHARP_DURATION);
			impact = MAX_IMPACT;
		}

		// metrics
		tornado_count = 0;

		total_tornado_damage = 0.0f;

		history.clear();

		update_history();
	}

	void BlackMage::reset(BlackMage& blm)
	{
		timeline = blm.timeline;

		gauge = blm.gauge;
		sharp = blm.sharp;
		impact = blm.impact;

		galeform_active = blm.galeform_active;
		tempestrike_gauge = blm.tempestrike_gauge;
		falcon_gauge = blm.falcon_gauge;

		in_air = blm.in_air;
		azure = blm.azure;
		prev_falcon_toss = blm.prev_falcon_toss;
		enhanced_galeform_next = blm.enhanced_galeform_next;

		// gauge
		gauge_timer = blm.gauge_timer;
		galeform_gauge_timer = blm.galeform_gauge_timer;
		inspire_gauge_timer = blm.inspire_gauge_timer;
		inspire_sharp_timer = blm.inspire_sharp_timer;

		// misc timers
		galeform_timer = blm.galeform_timer;
		sharp_timer = blm.sharp_timer;
		tornado_timer_1 = blm.tornado_timer_1;
		tornado_timer_2 = blm.tornado_timer_2;
		tornado_timer_3 = blm.tornado_timer_3;

		galeform_procs = blm.galeform_procs;

		// buffs
		chasing_step = blm.chasing_step;
		inspire = blm.inspire;
		typhoon_cleave = blm.typhoon_cleave;
		windfury = blm.windfury;
		galeform = blm.galeform;
		tempestrike = blm.tempestrike;
		divine_haste = blm.divine_haste;
		chasing_str = blm.chasing_str;
		set_bonus_dmg = blm.set_bonus_dmg;
		tornado_1 = blm.tornado_1;
		tornado_2 = blm.tornado_2;
		tornado_3 = blm.tornado_3;

		// cooldowns
		typhoon_cleave_cd = blm.typhoon_cleave_cd;
		falcon_toss_cd = blm.falcon_toss_cd;
		spear_thrust_cd = blm.spear_thrust_cd;

		// actions
		cast_timer = blm.cast_timer;
		action_timer = blm.action_timer;
		casting = blm.casting;
		cast_frame = blm.cast_frame;
		cast_speed = blm.cast_speed;

		// metrics
		tornado_count = 0;

		total_tornado_damage = 0.0f;

		history.clear();

		history.push_back(blm.history.back());
	}

	void BlackMage::update(int elapsed)
	{
		assert(elapsed > 0);

		// gauge ticks
		gauge_timer.update(elapsed);
		galeform_gauge_timer.update(elapsed);
		inspire_gauge_timer.update(elapsed);
		inspire_sharp_timer.update(elapsed);

		// misc timer
		galeform_timer.update(elapsed);
		sharp_timer.update(elapsed);
		tornado_timer_1.update(elapsed);
		tornado_timer_2.update(elapsed);
		tornado_timer_3.update(elapsed);

		// buffs
		chasing_step.update(elapsed);
		inspire.update(elapsed);
		typhoon_cleave.update(elapsed);
		windfury.update(elapsed);
		galeform.update(elapsed);
		tempestrike.update(elapsed);
		divine_haste.update(elapsed);
		chasing_str.update(elapsed);
		set_bonus_dmg.update(elapsed);
		tornado_1.update(elapsed);
		tornado_2.update(elapsed);
		tornado_3.update(elapsed);

		// cooldowns
		typhoon_cleave_cd.update(elapsed);
		falcon_toss_cd.update(elapsed);
		spear_thrust_cd.update(elapsed);

		// actions
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		//
		update_gauge();
		if (galeform_active && galeform.count == 0)
		{
			galeform_active = false;
			int extra_secs = tempestrike_gauge / 50.0f;
			tempestrike_gauge = 0.0f;
			tempestrike.reset(TEMPESTRIKE_BASE_DURATION + 1000 * extra_secs, 1);
			push_event(tempestrike.time);
		}
		if (sharp_timer.ready)
		{
			if (sharp == 0)
				sharp_timer.reset(0, false);
			else
			{
				sharp--;
				sharp_timer.reset(SHARP_DURATION, false);
				push_event(sharp_timer.time);
				gauge = std::min(gauge + GAUGE_PER_SHARP, MAX_GAUGE);
				impact = std::min(impact + 1, MAX_IMPACT);
				set_bonus_dmg.reset(SET_BONUS_DMG_DURATION, std::min(set_bonus_dmg.count + 1, 6));
				push_event(set_bonus_dmg.time);
			}
		}
		if (azure && windfury.count == 0)
			azure = false;
		if (galeform_timer.ready)
		{
			galeform_procs++;
			assert(galeform_timer.time == 0);
			assert(galeform_procs <= 2);
			galeform_timer.ready = false;
			if (galeform_procs < 2)
			{
				galeform_timer.time = GALEFORM_CD;
				push_event(GALEFORM_CD);
			}
		}
		if (cast_timer.ready)
			end_action();

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

			if (actions.empty())
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

	void BlackMage::update_gauge()
	{
		bool push_tick = false;
		if (gauge_timer.ready)
		{
			gauge = std::min(gauge + BASE_GAUGE_PER_TICK, MAX_GAUGE);
			gauge_timer.reset(TICK_TIMER, false);
			push_tick = true;
		}
		if (galeform_gauge_timer.ready)
		{
			if (galeform.count == 1)
				gauge = std::min(gauge + GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			else
				gauge = std::min(gauge + ENHANCED_GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			galeform_gauge_timer.reset(TICK_TIMER, false);
			push_tick = true;
		}
		if (inspire_gauge_timer.ready)
		{
			gauge = std::min(gauge + INSPIRE_GAUGE_PER_TICK, MAX_GAUGE);
			inspire_gauge_timer.reset(TICK_TIMER, false);
			push_tick = true;
		}
		if (inspire_sharp_timer.ready)
		{
			sharp++;
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			inspire_sharp_timer.reset(2 * TICK_TIMER, false);
			// don't need to push the tick because inspire_gauge_timer will handle it
		}
		if (tornado_timer_1.ready)
		{
			bool second_hit = tornado_1.time > TICK_TIMER;
			float damage = get_damage(second_hit ? TORNADO_HIT_2 : TORNADO_HIT_3);
			total_damage += damage;
			history.back().reward += damage;
			if (second_hit)
			{
				tornado_timer_1.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				tornado_timer_1.reset(0, false);
		}
		if (tornado_timer_2.ready)
		{
			bool second_hit = tornado_2.time > TICK_TIMER;
			float damage = get_damage(second_hit ? TORNADO_HIT_2 : TORNADO_HIT_3);
			total_damage += damage;
			history.back().reward += damage;
			if (second_hit)
			{
				tornado_timer_2.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				tornado_timer_2.reset(0, false);
		}
		if (tornado_timer_3.ready)
		{
			bool second_hit = tornado_3.time > TICK_TIMER;
			float damage = get_damage(second_hit ? TORNADO_HIT_2 : TORNADO_HIT_3);
			total_damage += damage;
			history.back().reward += damage;
			if (second_hit)
			{
				tornado_timer_3.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				tornado_timer_3.reset(0, false);
		}
		if (push_tick)
			push_event(TICK_TIMER);
	}

	float BlackMage::get_cast_speed() const
	{
		float haste = (stats.haste / (stats.haste + STAT_MOD)) + divine_haste.count * DIVINE_HASTE + inspire.count * INSPIRE_HASTE;
		return 1.0f + stats.base_atk_spd + haste * 1.6f;
	}

	bool BlackMage::can_use_action(int action) const
	{
		// only checked if actions aren't locked
		switch (action)
		{
		case BASIC_ATTACK:
			return !in_air;
		case SKYFALL:
			return SKYFALL_GAUGE_COST <= gauge;
		case BATTLE_CRY:
			return !in_air && typhoon_cleave_cd.ready;
		case TYPHOON_CLEAVE:
			return !in_air && typhoon_cleave.count > 0;
		case INSTANT_EDGE: 
			return INSTANT_EDGE_STACK_COST <= sharp;
		case FALCON_TOSS:
			return !azure && !prev_falcon_toss && falcon_toss_cd.ready && FALCON_TOSS_GAUGE_COST <= gauge;
		case AZURE_SEVERER:
			return azure && !prev_falcon_toss;
		case SHARP_IMPACT:
			return SHARP_IMPACT_STACK_COST <= impact;
		case GALEFORM:
			return galeform_procs > 0;
		case FALL:
			return in_air;
		case WAIT_FOR_GAUGE:
			return !in_air && gauge < MAX_GAUGE;
		}
		return false;
	}

	void BlackMage::use_action(int action)
	{
		int gauge_time;
		history.back().action = action;
		switch (action)
		{
		case BASIC_ATTACK:
			start_action(action, 0, BASIC_ATTACK_ANIM, true);
			break;
		case SKYFALL:
			if (!in_air)
				start_action(action, 0, SKYFALL_JUMP_ANIM, true);
			else
			{
				start_action(action, 1, SKYFALL_ANIM, true);
				gauge -= SKYFALL_GAUGE_COST;
				if (galeform_active)
					tempestrike_gauge += SKYFALL_GAUGE_COST;
				falcon_gauge += SKYFALL_GAUGE_COST;
				if (falcon_gauge >= 50.0f)
				{
					falcon_gauge -= 50.0f;
					if (falcon_toss_cd.time <= 1000)
						falcon_toss_cd.reset(0, true);
					else
					{
						falcon_toss_cd.reset(falcon_toss_cd.time - 1000, false);
						push_event(falcon_toss_cd.time);
					}
				}
				divine_haste.reset(DIVINE_HASTE_DURATION, std::min(divine_haste.count + 1, 5));
				push_event(divine_haste.time);
			}
			in_air = true;
			break;
		case BATTLE_CRY:
			// unaffected by cast speed
			cast_speed = 1.0f;
			start_action(action, 0, BATTLE_CRY_ANIM, false);
			typhoon_cleave_cd.reset(TYPHOON_CLEAVE_CD, false);
			push_event(typhoon_cleave_cd.time);
			typhoon_cleave.reset(TYPHOON_CLEAVE_DURATION, 1);
			push_event(typhoon_cleave.time);
			break;
		case TYPHOON_CLEAVE:
			start_action(action, 0, TYPHOON_CLEAVE_ANIM, true);
			typhoon_cleave.reset(0, 0);
			gauge = std::min(gauge + TYPHOON_CLEAVE_GAUGE, MAX_GAUGE);
			break;
		case INSTANT_EDGE:
			if (!in_air)
				start_action(action, 0, INSTANT_EDGE_ANIM_1, true);
			else
				start_action(action, 2, INSTANT_EDGE_ANIM_2, true);
			sharp -= INSTANT_EDGE_STACK_COST;
			gauge = std::min(gauge + GAUGE_PER_SHARP * INSTANT_EDGE_STACK_COST, MAX_GAUGE);
			impact = std::min(impact + INSTANT_EDGE_STACK_COST, MAX_IMPACT);
			set_bonus_dmg.reset(SET_BONUS_DMG_DURATION, std::min(set_bonus_dmg.count + INSTANT_EDGE_STACK_COST, 6));
			push_event(set_bonus_dmg.time);
			in_air = false;
			break;
		case FALCON_TOSS:
			if (!in_air)
				start_action(action, 0, FALCON_TOSS_ANIM_1, true);
			else
				start_action(action, 1, FALCON_TOSS_ANIM_2, true);
			falcon_toss_cd.reset(FALCON_TOSS_CD, false);
			gauge -= FALCON_TOSS_GAUGE_COST;
			if (galeform_active)
				tempestrike_gauge += FALCON_TOSS_GAUGE_COST;
			falcon_gauge += FALCON_TOSS_GAUGE_COST;
			if (falcon_gauge >= 50.0f)
			{
				falcon_gauge -= 50.0f;
				falcon_toss_cd.reset(falcon_toss_cd.time - 1000, false);
			}
			push_event(falcon_toss_cd.time);
			divine_haste.reset(DIVINE_HASTE_DURATION, std::min(divine_haste.count + 1, 5));
			push_event(divine_haste.time);
			in_air = true;
			prev_falcon_toss = true;
			return;
		case AZURE_SEVERER:
			if (!in_air)
				start_action(action, 0, FALCON_TOSS_ANIM_1, true);
			else
				start_action(action, 1, FALCON_TOSS_ANIM_2, true);
			azure = false;
			in_air = true;
			prev_falcon_toss = true;
			return;
		case SHARP_IMPACT:
			if (!in_air)
				start_action(action, 0, SHARP_IMPACT_GROUND_ANIM, true, SHARP_IMPACT_GROUND_FIXED_JUMP_ANIM);
			else
				start_action(action, 0, SHARP_IMPACT_AIR_ANIM, true);
			impact = 0;
			enhanced_galeform_next = true;
			in_air = false;
			break;
		case GALEFORM:
			start_action(action, 0, GALEFORM_ANIM, true);
			galeform_procs--;
			if (galeform_timer.time == 0)
			{
				assert(!galeform_timer.ready);
				galeform_timer.reset(GALEFORM_CD, false);
				push_event(galeform_timer.time);
			}
			break;
		case FALL:
			in_air = false;
			if (!prev_falcon_toss)
				action_timer.reset(FALL_ANIM, false);
			else
				action_timer.reset(FAST_FALL_ANIM, false);
			push_event(action_timer.time);
			break;
		case WAIT_FOR_GAUGE:
			gauge_time = gauge_timer.time;
			if (galeform_gauge_timer.time > 0)
				gauge_time = std::min(gauge_time, galeform_gauge_timer.time);
			if (inspire_gauge_timer.time > 0)
				gauge_time = std::min(gauge_time, inspire_gauge_timer.time);
			action_timer.reset(gauge_time, false);
			push_event(action_timer.time);
			break;
		}
		prev_falcon_toss = false;
	}

	void BlackMage::start_action(int action, int frame, int cast_time, bool start, int offset)
	{
		if (start)
			cast_speed = get_cast_speed();
		cast_timer.reset(cast_time / cast_speed + offset, false);
		push_event(cast_timer.time);
		action_timer.reset(0, false);
		casting = action;
		cast_frame = frame;
	}

	void BlackMage::end_action()
	{
		assert(cast_timer.time == 0);
		assert(cast_timer.ready);
		assert(casting >= 0);

		float damage = 0.0f;

		switch (casting)
		{
		case BASIC_ATTACK:
			damage = get_damage(casting);
			action_timer.reset(0, true);
			break;
		case SKYFALL:
			if (cast_frame == 0)
			{
				start_action(casting, 1, SKYFALL_ANIM, false);
				gauge -= SKYFALL_GAUGE_COST;
				if (galeform_active)
					tempestrike_gauge += SKYFALL_GAUGE_COST;
				falcon_gauge += SKYFALL_GAUGE_COST;
				if (falcon_gauge >= 50.0f)
				{
					falcon_gauge -= 50.0f;
					if (falcon_toss_cd.time <= 1000)
						falcon_toss_cd.reset(0, true);
					else
					{
						falcon_toss_cd.reset(falcon_toss_cd.time - 1000, false);
						push_event(falcon_toss_cd.time);
					}
				}
				divine_haste.reset(DIVINE_HASTE_DURATION, std::min(divine_haste.count + 1, 5));
				push_event(divine_haste.time);
				return;
			}
			else
			{
				damage = get_damage(casting);
				sharp = std::min(sharp + 1, 6);
				sharp_timer.reset(SHARP_DURATION, false);
				push_event(sharp_timer.time);
				if (prob(rng) < crit_rate)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + 1, 2));
					push_event(chasing_step.time);
				}
				if (prob(rng) < luck_rate)
				{
					damage += get_damage(SPEAR_THRUST);
					gauge = std::min(gauge + SPEAR_THRUST_GAUGE, MAX_GAUGE);
					spear_thrust_cd.reset(SPEAR_THRUST_CD, false);
					push_event(spear_thrust_cd.time);
				}
				if (sharp >= 2 || gauge >= SKYFALL_GAUGE_COST)
				{
					in_air = true;
					action_timer.reset(SKYFALL_ANIM_LOCK / cast_speed, false);
					push_event(action_timer.time);
				}
				else
				{
					in_air = false;
					action_timer.reset(SHORT_FALL_ANIM, false);
					push_event(action_timer.time);
				}
			}
			break;
		case BATTLE_CRY:
			sharp = std::min(sharp + 1, 6);
			sharp_timer.reset(SHARP_DURATION, false);
			gauge = std::min(gauge + INSPIRE_GAUGE_PER_TICK, MAX_GAUGE);
			inspire_gauge_timer.reset(TICK_TIMER, false);
			inspire_sharp_timer.reset(2 * TICK_TIMER, false);
			inspire.reset(INSPIRE_DURATION, 1);
			push_event(sharp_timer.time);
			push_event(inspire_gauge_timer.time);
			push_event(inspire_sharp_timer.time);
			push_event(inspire.time);
			action_timer.reset(0, true);
			break;
		case TYPHOON_CLEAVE:
			damage = get_damage(casting);
			action_timer.reset(TYPHOON_CLEAVE_ANIM_LOCK / cast_speed, false);
			push_event(action_timer.time);
			break;
		case INSTANT_EDGE:
			switch (cast_frame)
			{
			case 0:
				start_action(casting, 1, INSTANT_EDGE_ANIM_JUMP, false);
				damage = get_damage(casting, 0);
				if (windfury.count > 0)
					create_tornado();
				return;
			case 1:
				start_action(casting, 2, INSTANT_EDGE_ANIM_JUMP, false);
				return;
			case 2:
				damage = get_damage(casting, 1);
				if (windfury.count > 0)
					create_tornado();
				if (chasing_step.count > 0)
				{
					sharp = std::min(sharp + 1, 6);
					sharp_timer.reset(SHARP_DURATION, false);
					push_event(sharp_timer.time);
					impact = std::min(impact + INSTANT_EDGE_STACK_COST, MAX_IMPACT);
					if (chasing_step.count == 2)
					{
						chasing_step.reset(CHASE_DURATION, 1);
						push_event(chasing_step.time);
					}
					else
						chasing_step.reset(0, 0);
					chasing_str.reset(CHASING_STR_DURATION, std::min(chasing_str.count + 1, 2));
					push_event(chasing_str.time);
				}
				action_timer.reset(INSTANT_EDGE_ANIM_LOCK / cast_speed, true);
				break;
			}

			break;
		case FALCON_TOSS:
			damage = get_damage(casting, cast_frame);
			if (cast_frame == 0)
			{
				start_action(casting, 1, FALCON_TOSS_ANIM_2, false);
				return;
			}
			action_timer.reset(0, true);
			break;
		case AZURE_SEVERER:
			damage = get_damage(casting, cast_frame);
			if (cast_frame == 0)
			{
				start_action(casting, 1, FALCON_TOSS_ANIM_2, false);
				return;
			}
			else if (windfury.count > 0)
				create_tornado();
			action_timer.reset(0, true);
			break;
		case SHARP_IMPACT:
			damage = get_damage(casting);
			sharp = 6;
			sharp_timer.reset(SHARP_DURATION, false);
			windfury.reset(WINDFURY_DURATION, 1);
			action_timer.reset(SHARP_IMPACT_ANIM_LOCK / cast_speed, false);
			push_event(sharp_timer.time);
			push_event(windfury.time);
			push_event(action_timer.time);
			break;
		case GALEFORM:
			if (!in_air)
				damage = get_damage(casting);
			galeform_active = true;
			galeform.reset(GALEFORM_DURATION, enhanced_galeform_next ? 1 : 2);
			enhanced_galeform_next = false;
			if (enhanced_galeform_next)
				gauge = std::min(gauge + ENHANCED_GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			else
				gauge = std::min(gauge + GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			galeform_gauge_timer.reset(TICK_TIMER, false);
			push_event(galeform_gauge_timer.time);
			action_timer.reset(0, true);
			break;
		}
		if (damage > 0.0f)
		{
			total_damage += damage;
			history.back().reward += damage;
			//update_metric(casting, damage);
		}
		casting = -1;
		cast_frame = -1;
		cast_timer.ready = false;
	}

	void BlackMage::create_tornado()
	{
		if (tornado_1.count && tornado_2.count && tornado_3.count)
			return;
		if (!tornado_1.count)
		{
			tornado_1.reset(TORNADO_DURATION, 1);
			tornado_timer_1.reset(TICK_TIMER, false);
		}
		else if (!tornado_2.count)
		{
			tornado_2.reset(TORNADO_DURATION, 1);
			tornado_timer_2.reset(TICK_TIMER, false);
		}
		else
		{
			tornado_3.reset(TORNADO_DURATION, 1);
			tornado_timer_3.reset(TICK_TIMER, false);
		}
		float damage = get_damage(TORNADO_HIT_1);
		total_damage += damage;
		history.back().reward += damage;
		push_event(TORNADO_DURATION);
		push_event(TICK_TIMER);
	}

	float BlackMage::get_damage(int action, int hit)
	{
		float potency = 0.0f;
		float skill_atk = 0.0f;
		float dmg = 1.0f;
		float ele_dmg = 1.0f;
		bool roll_luck = false;
		bool double_skill_crit = false;
		switch (action)
		{
		case BASIC_ATTACK:
			potency = BASIC_ATTACK_POTENCY;
			skill_atk = BASIC_ATTACK_ATK;
			roll_luck = true;
			break;
		case SKYFALL:
			potency = SKYFALL_POTENCY;
			skill_atk = SKYFALL_ATK;
			dmg += galeform.count * GALEFORM_BONUS_DMG;
			ele_dmg += SET_ELE_DMG;
			roll_luck = true;
			break;
		case TYPHOON_CLEAVE:
			potency = TYPHOON_CLEAVE_POTENCY;
			skill_atk = TYPHOON_CLEAVE_ATK;
			dmg += EXP_SKILL_DMG;
			roll_luck = true;
			break;
		case INSTANT_EDGE:
			if (hit == 0)
			{
				potency = INSTANT_EDGE_POTENCY_1;
				skill_atk = INSTANT_EDGE_ATK_1;
			}
			else
			{
				potency = INSTANT_EDGE_POTENCY_2;
				skill_atk = INSTANT_EDGE_ATK_2;
				roll_luck = true;
			}
			dmg += INSTANT_EDGE_BREAK_DMG;
			dmg += EXP_SKILL_DMG;
			dmg += galeform.count * GALEFORM_BONUS_DMG;
			ele_dmg += SET_ELE_DMG;
			if (chasing_step.count > 0)
				double_skill_crit = true;
			break;
		case FALCON_TOSS:
			if (hit == 0)
			{
				potency = FALCON_TOSS_POTENCY_1;
				skill_atk = FALCON_TOSS_ATK_1;
			}
			else
			{
				potency = FALCON_TOSS_POTENCY_2;
				skill_atk = FALCON_TOSS_ATK_2;
				roll_luck = true;
			}
			dmg += EXP_SKILL_DMG;
			break;
		case AZURE_SEVERER:
			if (hit == 0)
			{
				potency = FALCON_TOSS_POTENCY_1 * ENHANCED_MULTIPLIER;
				skill_atk = FALCON_TOSS_ATK_1 * ENHANCED_MULTIPLIER;
			}
			else
			{
				potency = FALCON_TOSS_POTENCY_2 * ENHANCED_MULTIPLIER;
				skill_atk = FALCON_TOSS_ATK_2 * ENHANCED_MULTIPLIER;
				roll_luck = true;
			}
			dmg += EXP_SKILL_DMG;
			break;
		case SHARP_IMPACT:
			potency = SHARP_IMPACT_POTENCY * ENHANCED_MULTIPLIER;
			skill_atk = SHARP_IMPACT_ATK * ENHANCED_MULTIPLIER;
			dmg += EXP_SKILL_DMG;
			roll_luck = true;
			if (chasing_step.count > 0)
				double_skill_crit = true;
			break;
		case GALEFORM:
			potency = GALEFORM_POTENCY;
			skill_atk = GALEFORM_ATK;
			dmg += EXP_SKILL_DMG;
			roll_luck = true;
			break;
		case SPEAR_THRUST:
			potency = SPEAR_THRUST_POTENCY;
			dmg += SPEAR_THRUST_BONUS_DMG;
			dmg += luck_rate;
			roll_luck = true;
			break;
		case TORNADO_HIT_1:
			potency = TORNADO_POTENCY_1;
			dmg += TORNADO_BONUS_DMG;
			break;
		case TORNADO_HIT_2:
			potency = TORNADO_POTENCY_2;
			dmg += TORNADO_BONUS_DMG;
			break;
		case TORNADO_HIT_3:
			potency = TORNADO_POTENCY_3;
			dmg += TORNADO_BONUS_DMG;
			break;
		case BATTLE_CRY:
		case FALL:
		case WAIT_FOR_GAUGE:
			throw 123;
		}
		if (windfury.count)
			dmg += WINDFURY_BONUS_DMG;
		dmg += set_bonus_dmg.count * SET_BONUS_DMG;

		float str = stats.str + galeform.count * GALEFORM_FLAT_STR;
		float str_percent = 1.0f + tempestrike.count * TEMPESTRIKE_STR + chasing_str.count * CHASING_STR + galeform.count * GALEFORM_STR;
		str *= str_percent;

		float atk_percent = 1.0f + (sharp > 0 ? SHARP_ATK : 0.0f);
		// 0.70f for armor
		float total_atk = (stats.flat_atk + str * 0.725f) * atk_percent;
		float atk = 0.70f * total_atk + stats.refined_atk;

		dmg += VULN_DMG;
		ele_dmg += (0.06f + (stats.mastery / (stats.mastery + STAT_MOD))) * 0.65f;
		float vers_dmg = 1.0f + (stats.vers / (stats.vers + STAT_MOD)) * 0.35f;

		// * dmg * ele_dmg * vers_dmg
		float skill_dmg = (atk * potency + skill_atk) * dmg * ele_dmg * vers_dmg;

		// skill crit dmg
		skill_dmg *= 1.0f + (double_skill_crit ? 2.0f : 1.0f) * crit_rate * crit_multi;

		// instant edge combo based on skill_dmg
		if (action == INSTANT_EDGE && hit == 1)
			skill_dmg *= 1.0f + luck_rate * luck_multi;

		// lucky strike dmg
		if (roll_luck)
			skill_dmg += luck_rate * (total_atk + stats.refined_atk) * (dmg + luck_rate) * ele_dmg * vers_dmg * luck_multi * (1.0f + crit_rate * crit_multi);

		return skill_dmg;
	}

	void BlackMage::get_state(float* state)
	{
		state[0] = gauge / MAX_GAUGE;
		state[1] = sharp > 0;
		state[2] = sharp > 1;
		state[3] = sharp > 2;
		state[4] = sharp > 3;
		state[5] = sharp > 4;
		state[6] = sharp > 5;
		state[7] = impact / (float)MAX_IMPACT;
		state[8] = tempestrike_gauge / 800.0f; // extra 16 seconds
		state[9] = falcon_gauge / 50.0f;
		state[10] = in_air;
		state[11] = azure;
		state[12] = prev_falcon_toss;
		state[13] = enhanced_galeform_next;
		state[14] = gauge_timer.time / (float)TICK_TIMER;
		state[15] = galeform_gauge_timer.time / (float)TICK_TIMER;
		state[16] = inspire_gauge_timer.time / (float)TICK_TIMER;
		state[17] = inspire_sharp_timer.time / (float)(2.0f * TICK_TIMER);
		state[18] = galeform_procs > 0;
		state[19] = galeform_procs > 1;
		state[20] = (GALEFORM_CD - galeform_timer.time) / (float)GALEFORM_CD;
		state[21] = (SHARP_DURATION - sharp_timer.time) / (float)SHARP_DURATION;
		state[22] = tornado_timer_1.time / (float)TICK_TIMER;
		state[23] = tornado_timer_2.time / (float)TICK_TIMER;
		state[24] = tornado_timer_3.time / (float)TICK_TIMER;
		state[25] = chasing_step.count > 0;
		state[26] = chasing_step.count > 1;
		state[27] = chasing_step.time / (float)CHASE_DURATION;
		state[28] = inspire.count > 0;
		state[29] = inspire.time / (float)INSPIRE_DURATION;
		state[30] = windfury.count > 0;
		state[31] = windfury.time / (float)WINDFURY_DURATION;
		state[32] = galeform.count > 0;
		state[33] = galeform.count > 1;
		state[34] = galeform.time / (float)GALEFORM_DURATION;
		state[35] = tempestrike.count > 0;
		state[36] = tempestrike.time / (float)(3.0f * TEMPESTRIKE_BASE_DURATION);
		state[37] = divine_haste.count / 5.0f;
		state[38] = divine_haste.time / (float)DIVINE_HASTE_DURATION;
		state[39] = chasing_str.count / 2.0f;
		state[40] = chasing_str.time / (float)CHASING_STR_DURATION;
		state[41] = set_bonus_dmg.count / 6.0f;
		state[42] = set_bonus_dmg.time / (float)SET_BONUS_DMG_DURATION;
		state[43] = tornado_1.count > 0;
		state[44] = tornado_1.time / (float)TORNADO_DURATION;
		state[45] = tornado_2.count > 0;
		state[46] = tornado_2.time / (float)TORNADO_DURATION;
		state[47] = tornado_3.count > 0;
		state[48] = tornado_3.time / (float)TORNADO_DURATION;
		state[49] = typhoon_cleave_cd.ready;
		state[50] = typhoon_cleave_cd.time / (float)TYPHOON_CLEAVE_CD;
		state[51] = falcon_toss_cd.ready;
		state[52] = falcon_toss_cd.time / (float)FALCON_TOSS_CD;
		state[53] = spear_thrust_cd.ready;
		state[54] = spear_thrust_cd.time / (float)SPEAR_THRUST_CD;
	}

	std::string BlackMage::get_info()
	{
		return "\n";
	}
}