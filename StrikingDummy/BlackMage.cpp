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

		luck_rate = 0.05f + stats.luck / (stats.luck + STAT_MOD);
		luck_multi = (0.40f + stats.base_luck_multi + 0.25f * luck_rate) * LUCKY_STRIKE_MULTIPLIER;
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
		falcon_toss_timer.reset(0, false);
		muku_chief_timer.reset(0, false);

		galeform_procs = 2;
		falcon_toss_procs = MAX_FALCON_TOSS_PROCS;
		muku_chief_procs = 2;

		// buffs
		chasing_step.reset(0, 0);
		inspire.reset(0, 0);
		typhoon_cleave.reset(0, 0);
		windfury.reset(0, 0);
		galeform.reset(0, 0);
		tempestrike.reset(0, 0);
		divine_haste.reset(0, 0);
		chasing_str.reset(0, 0);
		tornado_1.reset(0, 0);
		tornado_2.reset(0, 0);
		tornado_3.reset(0, 0);
		muku_chief.reset(0, 0);
		celestial_flier.reset(0, 0);

		// cooldowns
		typhoon_cleave_cd.reset(0, true);
		spear_thrust_cd.reset(0, true);
		celestial_flier_cd.reset(0, true);

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
		total_damage = 0.0f;

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
		falcon_toss_timer = blm.falcon_toss_timer;
		muku_chief_timer = blm.muku_chief_timer;

		galeform_procs = blm.galeform_procs;
		falcon_toss_procs = blm.falcon_toss_procs;
		muku_chief_procs = blm.muku_chief_procs;

		// buffs
		chasing_step = blm.chasing_step;
		inspire = blm.inspire;
		typhoon_cleave = blm.typhoon_cleave;
		windfury = blm.windfury;
		galeform = blm.galeform;
		tempestrike = blm.tempestrike;
		divine_haste = blm.divine_haste;
		chasing_str = blm.chasing_str;
		tornado_1 = blm.tornado_1;
		tornado_2 = blm.tornado_2;
		tornado_3 = blm.tornado_3;
		muku_chief = blm.muku_chief;
		celestial_flier = blm.celestial_flier;

		// cooldowns
		typhoon_cleave_cd = blm.typhoon_cleave_cd;
		spear_thrust_cd = blm.spear_thrust_cd;
		celestial_flier_cd = blm.celestial_flier_cd;

		// actions
		cast_timer = blm.cast_timer;
		action_timer = blm.action_timer;
		casting = blm.casting;
		cast_frame = blm.cast_frame;
		cast_speed = blm.cast_speed;

		// metrics
		tornado_count = 0;

		total_tornado_damage = 0.0f;
		total_damage = 0.0f;

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
		falcon_toss_timer.update(elapsed);
		muku_chief_timer.update(elapsed);

		// buffs
		chasing_step.update(elapsed);
		inspire.update(elapsed);
		typhoon_cleave.update(elapsed);
		windfury.update(elapsed);
		galeform.update(elapsed);
		tempestrike.update(elapsed);
		divine_haste.update(elapsed);
		chasing_str.update(elapsed);
		tornado_1.update(elapsed);
		tornado_2.update(elapsed);
		tornado_3.update(elapsed);
		muku_chief.update(elapsed);
		celestial_flier.update(elapsed);

		// cooldowns
		typhoon_cleave_cd.update(elapsed);

		spear_thrust_cd.update(elapsed);
		celestial_flier_cd.update(elapsed);

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
		if (falcon_toss_timer.ready)
		{
			falcon_toss_procs++;
			assert(falcon_toss_timer.time == 0);
			assert(falcon_toss_procs <= MAX_FALCON_TOSS_PROCS);
			falcon_toss_timer.ready = false;
			if (falcon_toss_procs < MAX_FALCON_TOSS_PROCS)
			{
				falcon_toss_timer.time = FALCON_TOSS_CD;
				push_event(FALCON_TOSS_CD);
			}
		}
		if (muku_chief_timer.ready)
		{
			muku_chief_procs++;
			assert(muku_chief_timer.time == 0);
			assert(muku_chief_procs <= 2);
			muku_chief_timer.ready = false;
			if (muku_chief_procs < 2)
			{
				muku_chief_timer.time = MUKU_CHIEF_CD;
				push_event(MUKU_CHIEF_CD);
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
			if (galeform.time > TICK_TIMER)
			{
				galeform_gauge_timer.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				galeform_gauge_timer.reset(0, false);
		}
		if (inspire_gauge_timer.ready)
		{
			gauge = std::min(gauge + INSPIRE_GAUGE_PER_TICK, MAX_GAUGE);
			if (inspire.time > TICK_TIMER)
			{
				inspire_gauge_timer.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				inspire_gauge_timer.reset(0, false);
		}
		if (inspire_sharp_timer.ready)
		{
			sharp++;
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			if (inspire.time > 2 * TICK_TIMER)
				// don't need to push the tick because inspire_gauge_timer will handle it
				inspire_sharp_timer.reset(2 * TICK_TIMER, false);
			else
				inspire_sharp_timer.reset(0, false);
		}
		if (tornado_timer_1.ready)
		{
			float damage = get_damage(TORNADO_HIT);
			total_damage += damage;
			history.back().reward += damage;
			total_tornado_damage += damage;
			if (tornado_1.time > TICK_TIMER)
			{
				tornado_timer_1.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				tornado_timer_1.reset(0, false);
		}
		if (tornado_timer_2.ready)
		{
			float damage = get_damage(TORNADO_HIT);
			total_damage += damage;
			history.back().reward += damage;
			total_tornado_damage += damage;
			if (tornado_2.time > TICK_TIMER)
			{
				tornado_timer_2.reset(TICK_TIMER, false);
				push_tick = true;
			}
			else
				tornado_timer_2.reset(0, false);
		}
		if (tornado_timer_3.ready)
		{
			float damage = get_damage(TORNADO_HIT);
			total_damage += damage;
			history.back().reward += damage;
			total_tornado_damage += damage;
			if (tornado_3.time > TICK_TIMER)
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
		float haste = (stats.haste / (stats.haste + STAT_MOD)) + divine_haste.count * DIVINE_HASTE + inspire.count * INSPIRE_HASTE + celestial_flier.count * CELESTIAL_FLIER_HASTE_PERCENT;
		return 1.0f + stats.base_atk_spd + haste * 1.6f;
	}

	bool BlackMage::can_use_action(int action) const
	{
		// only checked if actions aren't locked
		switch (action)
		{
		case BASIC_ATTACK:
			//return !in_air;
			return false;
		case SKYFALL:
			return SKYFALL_GAUGE_COST <= gauge;
		case BATTLE_CRY:
			return ENABLE_BATTLE_CRY_TALENT && !in_air && typhoon_cleave_cd.ready;
		case TYPHOON_CLEAVE:
			if (ENABLE_BATTLE_CRY_TALENT)
				return typhoon_cleave.count > 0;
			else
				return !in_air && typhoon_cleave_cd.ready;
		case INSTANT_EDGE: 
			return INSTANT_EDGE_STACK_COST <= sharp;
		case FALCON_TOSS:
			return !azure && !prev_falcon_toss && falcon_toss_procs > 0 && FALCON_TOSS_GAUGE_COST <= gauge;
		case AZURE_SEVER:
			return azure && !prev_falcon_toss;
		case SHARP_IMPACT:
			return SHARP_IMPACT_STACK_COST <= impact;
		case GALEFORM:
			return galeform_procs > 0;
		case FALL:
			return in_air;
		case WAIT_FOR_GAUGE:
			return !in_air && gauge < MAX_GAUGE;
		case MUKU_CHIEF:
			return !in_air && muku_chief_procs > 0;
		case CELESTIAL_FLIER:
			return !in_air && celestial_flier_cd.ready;
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
					if (falcon_toss_procs < MAX_FALCON_TOSS_PROCS)
					{
						if (falcon_toss_timer.time <= 1000)
						{
							falcon_toss_procs++;
							if (falcon_toss_procs == MAX_FALCON_TOSS_PROCS)
								falcon_toss_timer.reset(0, false);
							else
							{
								falcon_toss_timer.reset(FALCON_TOSS_CD + falcon_toss_timer.time - 1000, false);
								push_event(falcon_toss_timer.time);
							}
						}
						else
						{
							falcon_toss_timer.reset(falcon_toss_timer.time - 1000, false);
							push_event(falcon_toss_timer.time);
						}
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
			start_action(action, 0, TYPHOON_CLEAVE_ANIM, true, in_air ? FALL_ANIM : 0);
			typhoon_cleave.reset(0, 0);
			gauge = std::min(gauge + TYPHOON_CLEAVE_GAUGE, MAX_GAUGE);
			if (!ENABLE_BATTLE_CRY_TALENT)
			{
				typhoon_cleave_cd.reset(TYPHOON_CLEAVE_CD, false);
				push_event(typhoon_cleave_cd.time);
			}
			in_air = false;
			break;
		case INSTANT_EDGE:
			if (!in_air)
				start_action(action, 0, INSTANT_EDGE_ANIM_1, true);
			else
				start_action(action, 2, INSTANT_EDGE_ANIM_2, true);
			sharp -= INSTANT_EDGE_STACK_COST;
			gauge = std::min(gauge + GAUGE_PER_SHARP * INSTANT_EDGE_STACK_COST, MAX_GAUGE);
			impact = std::min(impact + INSTANT_EDGE_STACK_COST, MAX_IMPACT);
			in_air = false;
			break;
		case FALCON_TOSS:
			if (!in_air)
				start_action(action, 0, FALCON_TOSS_ANIM_1, true);
			else
				start_action(action, 1, FALCON_TOSS_ANIM_2, true);
			gauge -= FALCON_TOSS_GAUGE_COST;
			if (galeform_active)
				tempestrike_gauge += FALCON_TOSS_GAUGE_COST;
			falcon_gauge += FALCON_TOSS_GAUGE_COST;
			falcon_toss_procs--;
			if (falcon_toss_timer.time == 0)
			{
				assert(!falcon_toss_timer.ready);
				falcon_toss_timer.reset(FALCON_TOSS_CD, false);
			}
			if (falcon_gauge >= 50.0f)
			{
				falcon_gauge -= 50.0f;
				if (falcon_toss_procs < MAX_FALCON_TOSS_PROCS)
				{
					if (falcon_toss_timer.time <= 1000)
					{
						falcon_toss_procs++;
						if (falcon_toss_procs == MAX_FALCON_TOSS_PROCS)
							falcon_toss_timer.reset(0, false);
						else
							falcon_toss_timer.reset(FALCON_TOSS_CD + falcon_toss_timer.time - 1000, false);
					}
					else
						falcon_toss_timer.reset(falcon_toss_timer.time - 1000, false);
				}
			}
			push_event(falcon_toss_timer.time);
			divine_haste.reset(DIVINE_HASTE_DURATION, std::min(divine_haste.count + 1, 5));
			push_event(divine_haste.time);
			sharp = std::min(sharp + 1, 6);
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			in_air = true;
			prev_falcon_toss = true;
			falcon_crit = false;
			return;
		case AZURE_SEVER:
			if (!in_air)
				start_action(action, 0, FALCON_TOSS_ANIM_1, true);
			else
				start_action(action, 1, FALCON_TOSS_ANIM_2, true);
			sharp = std::min(sharp + 3, 6);
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			azure = false;
			in_air = true;
			prev_falcon_toss = true;
			return;
		case SHARP_IMPACT:
			if (!in_air)
				start_action(action, 0, SHARP_IMPACT_GROUND_ANIM, true, SHARP_IMPACT_GROUND_FIXED_JUMP_ANIM);
			else
				start_action(action, 0, SHARP_IMPACT_AIR_ANIM, true);
			sharp = std::min(sharp + SHARP_IMPACT_SHARP, 6);
			sharp_timer.reset(SHARP_DURATION, false);
			windfury.reset(WINDFURY_DURATION, 1);
			azure = true;
			impact = 0;
			enhanced_galeform_next = true;
			in_air = false;
			push_event(sharp_timer.time);
			push_event(windfury.time);
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
				action_timer.reset(FALL_ANIM + ACTION_TAX, false);
			else
				action_timer.reset(FAST_FALL_ANIM + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case WAIT_FOR_GAUGE:
			gauge_time = gauge_timer.time;
			if (galeform_gauge_timer.time > 0)
				gauge_time = std::min(gauge_time, galeform_gauge_timer.time);
			if (inspire_gauge_timer.time > 0)
				gauge_time = std::min(gauge_time, inspire_gauge_timer.time);
			action_timer.reset(gauge_time + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case MUKU_CHIEF:
			// unaffected by cast speed
			cast_speed = 1.0f;
			start_action(action, 0, IMAGINE_FIXED_ANIM, false);
			muku_chief_procs--;
			if (muku_chief_timer.time == 0)
			{
				assert(!muku_chief_timer.ready);
				muku_chief_timer.reset(MUKU_CHIEF_CD, false);
				push_event(muku_chief_timer.time);
			}
			break;
		case CELESTIAL_FLIER:
			// unaffected by cast speed
			cast_speed = 1.0f;
			start_action(action, 0, IMAGINE_FIXED_ANIM, false);
			celestial_flier_cd.reset(CELESTIAL_FLIER_CD, false);
			push_event(celestial_flier_cd.time);
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
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
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
					if (falcon_toss_procs < MAX_FALCON_TOSS_PROCS)
					{
						if (falcon_toss_timer.time <= 1000)
						{
							falcon_toss_procs++;
							if (falcon_toss_procs == MAX_FALCON_TOSS_PROCS)
								falcon_toss_timer.reset(0, false);
							else
							{
								falcon_toss_timer.reset(FALCON_TOSS_CD + falcon_toss_timer.time - 1000, false);
								push_event(falcon_toss_timer.time);
							}
						}
						else
						{
							falcon_toss_timer.reset(falcon_toss_timer.time - 1000, false);
							push_event(falcon_toss_timer.time);
						}
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
				float crit = stats.crit + MUKU_CHIEF_CRIT * muku_chief.count;
				float crit_rate = 0.05f + crit / (crit + STAT_MOD);
				if (prob(rng) < crit_rate)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + 1, 2));
					push_event(chasing_step.time);
				}
				if (spear_thrust_cd.ready && prob(rng) < luck_rate)
				{
					damage += get_damage(SPEAR_THRUST);
					gauge = std::min(gauge + SPEAR_THRUST_GAUGE, MAX_GAUGE);
					spear_thrust_cd.reset(SPEAR_THRUST_CD, false);
					push_event(spear_thrust_cd.time);
				}
				if (sharp >= 2 || gauge >= SKYFALL_GAUGE_COST)
				{
					in_air = true;
					action_timer.reset(SKYFALL_ANIM_LOCK / cast_speed + ACTION_TAX, false);
				}
				else
				{
					in_air = false;
					action_timer.reset(SHORT_FALL_ANIM + ACTION_TAX, false);
				}
				push_event(action_timer.time);
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
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case TYPHOON_CLEAVE:
			damage = get_damage(casting);
			action_timer.reset(TYPHOON_CLEAVE_ANIM_LOCK / cast_speed + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case INSTANT_EDGE:
			switch (cast_frame)
			{
			case 0:
				start_action(casting, 1, INSTANT_EDGE_ANIM_JUMP, false);
				damage = get_damage(casting, 0);
				total_damage += damage;
				history.back().reward += damage;
				if (windfury.count > 0)
					create_tornado();
				return;
			case 1:
				start_action(casting, 2, INSTANT_EDGE_ANIM_2, false);
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
				action_timer.reset(INSTANT_EDGE_ANIM_LOCK / cast_speed + ACTION_TAX, false);
				push_event(action_timer.time);
				break;
			}
			break;
		case FALCON_TOSS:
		{
			damage = get_damage(casting, cast_frame);
			if (!falcon_crit)
			{
				float crit = stats.crit + MUKU_CHIEF_CRIT * muku_chief.count;
				float crit_rate = 0.05f + crit / (crit + STAT_MOD);
				if (prob(rng) < crit_rate)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + 1, 2));
					push_event(chasing_step.time);
					falcon_crit = true;
				}
			}
			if (cast_frame == 0)
			{
				start_action(casting, 1, FALCON_TOSS_ANIM_2, false);
				total_damage += damage;
				history.back().reward += damage;
				return;
			}
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
		}
			break;
		case AZURE_SEVER:
			damage = get_damage(casting, cast_frame);
			if (cast_frame == 0)
			{
				start_action(casting, 1, FALCON_TOSS_ANIM_2, false);
				total_damage += damage;
				history.back().reward += damage;
				return;
			}
			else if (windfury.count > 0)
					create_tornado();
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case SHARP_IMPACT:
			damage = get_damage(casting);
			action_timer.reset(SHARP_IMPACT_ANIM_LOCK / cast_speed + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case GALEFORM:
			if (!in_air)
				damage = get_damage(casting);
			galeform_active = true;
			galeform.reset(GALEFORM_DURATION, enhanced_galeform_next ? 2 : 1);
			push_event(galeform.time);
			enhanced_galeform_next = false;
			if (enhanced_galeform_next)
				gauge = std::min(gauge + ENHANCED_GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			else
				gauge = std::min(gauge + GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			galeform_gauge_timer.reset(TICK_TIMER, false);
			push_event(galeform_gauge_timer.time);
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case MUKU_CHIEF:
			muku_chief.reset(IMAGINE_DURATION, 1);
			push_event(muku_chief.time);
			action_timer.reset(IMAGINE_ANIM_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case CELESTIAL_FLIER:
			celestial_flier.reset(IMAGINE_DURATION, 1);
			push_event(celestial_flier.time);
			action_timer.reset(IMAGINE_ANIM_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
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
		float damage = get_damage(TORNADO_HIT);
		total_damage += damage;
		history.back().reward += damage;
		push_event(TORNADO_DURATION);
		push_event(TICK_TIMER);
		tornado_count++;
		total_tornado_damage += damage;
	}

	float BlackMage::get_damage(int action, int hit)
	{
		float potency = 0.0f;
		float skill_atk = 0.0f;
		float dmg = 1.0f;
		float ele_dmg = 1.0f;
		bool expertise = false;
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
			dmg += galeform.count > 0 ? GALEFORM_BONUS_DMG : 0.0f;
			roll_luck = true;
			break;
		case TYPHOON_CLEAVE:
			potency = TYPHOON_CLEAVE_POTENCY;
			skill_atk = TYPHOON_CLEAVE_ATK;
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
			expertise = true;
			dmg += galeform.count > 0 ? GALEFORM_BONUS_DMG : 0.0f;
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
			expertise = true;
			break;
		case AZURE_SEVER:
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
			expertise = true;
			break;
		case SHARP_IMPACT:
			potency = SHARP_IMPACT_POTENCY;
			skill_atk = SHARP_IMPACT_ATK;
			expertise = true;
			roll_luck = true;
			if (chasing_step.count > 0)
				double_skill_crit = true;
			break;
		case GALEFORM:
			potency = GALEFORM_POTENCY;
			skill_atk = GALEFORM_ATK;
			expertise = true;
			roll_luck = true;
			break;
		case SPEAR_THRUST:
			potency = SPEAR_THRUST_POTENCY;
			dmg += SPEAR_THRUST_BONUS_DMG;
			dmg += luck_rate;
			roll_luck = true;
			break;
		case TORNADO_HIT:
			potency = TORNADO_POTENCY;
			dmg += TORNADO_BONUS_DMG;
			roll_luck = ENABLE_LUCK_TORNADO_FACTOR;
			break;
		case BATTLE_CRY:
		case FALL:
		case WAIT_FOR_GAUGE:
			throw 123;
		}
		if (windfury.count)
			dmg += WINDFURY_BONUS_DMG;
		if (expertise)
			dmg += EXP_SKILL_DMG;

		float str = stats.str + (galeform.count > 0 ? GALEFORM_FLAT_STR : 0.0f);
		float str_percent = 1.0f + tempestrike.count * TEMPESTRIKE_STR + chasing_str.count * CHASING_STR + (galeform.count > 0 ? GALEFORM_STR : 0.0f) + stats.base_str_per;
		str *= str_percent;

		float atk_percent = 1.0f + (sharp > 0 ? SHARP_ATK : 0.0f) + stats.base_atk_per;
		float total_atk = (stats.flat_atk + str * 0.725f) * atk_percent;
		float armor_pen = stats.base_armor_pen;
		float base_armor = 2786.0f * (1.0f - armor_pen);
		float armor = base_armor / (base_armor + 6500.0f);
		float atk = (1.0f - armor) * total_atk + stats.refined_atk;

		dmg += VULN_DMG + stats.base_dmg;
		ele_dmg += (0.06f + (stats.mastery / (stats.mastery + STAT_MOD))) * 0.65f + (stats.base_ele_stat / (stats.base_ele_stat + ELE_MOD)) + (stats.base_serum_stat / (stats.base_serum_stat + ELE_MOD));
		float vers_dmg = 1.0f + (stats.vers / (stats.vers + VERS_MOD)) * 0.35f;

		float crit = stats.crit + MUKU_CHIEF_CRIT * muku_chief.count;
		float crit_rate = 0.05f + crit / (crit + STAT_MOD);
		float eff_crit_rate = std::min((double_skill_crit ? 2.0f : 1.0f) * crit_rate, 1.0f);
		float crit_multi = 0.50f + stats.base_crit_multi + MUKU_CHIEF_CRIT_MULTI * muku_chief.count;

		// * dmg * ele_dmg * vers_dmg
		float skill_dmg = (atk * potency + skill_atk) * dmg * ele_dmg * vers_dmg;
		
		if (expertise)
		{
			skill_dmg *= 1.0f - eff_crit_rate;
			armor_pen += 0.50f;
			base_armor = 2786.0f * (1.0f - armor_pen);
			armor = base_armor / (base_armor + 6500.0f);
			atk = (1.0f - armor) * total_atk + stats.refined_atk;
			skill_dmg += (atk * potency + skill_atk) * dmg * ele_dmg * vers_dmg * eff_crit_rate * (1.0f + crit_multi);
		}
		else
			// skill crit dmg
			skill_dmg *= 1.0f + eff_crit_rate * crit_multi;

		// instant edge combo based on skill_dmg
		if (ENABLE_INSTANT_EDGE_COMBO_TALENT && action == INSTANT_EDGE && hit == 1)
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
		state[8] = in_air;
		state[9] = azure;
		state[10] = prev_falcon_toss;
		state[11] = enhanced_galeform_next;
		state[12] = galeform_procs > 0;
		state[13] = galeform_procs > 1;
		state[14] = galeform_timer.time / (float)GALEFORM_CD;
		state[15] = muku_chief_procs > 0;
		state[16] = muku_chief_procs > 1;
		state[17] = muku_chief_timer.time / (float)MUKU_CHIEF_CD;
		state[18] = (SHARP_DURATION - sharp_timer.time) / (float)SHARP_DURATION;
		state[19] = chasing_step.count > 0;
		state[20] = chasing_step.count > 1;
		state[21] = chasing_step.time / (float)CHASE_DURATION;
		state[22] = inspire.count > 0;
		state[23] = inspire.time / (float)INSPIRE_DURATION;
		state[24] = windfury.count > 0;
		state[25] = windfury.time / (float)WINDFURY_DURATION;
		state[26] = galeform.count > 0;
		state[27] = galeform.count > 1;
		state[28] = galeform.time / (float)GALEFORM_DURATION;
		state[29] = tornado_1.count > 0;
		state[30] = tornado_1.time / (float)TORNADO_DURATION;
		state[31] = tornado_2.count > 0;
		state[32] = tornado_2.time / (float)TORNADO_DURATION;
		state[33] = tornado_3.count > 0;
		state[34] = tornado_3.time / (float)TORNADO_DURATION;
		state[35] = typhoon_cleave_cd.ready;
		state[36] = typhoon_cleave_cd.time / (float)TYPHOON_CLEAVE_CD;
		state[37] = gauge_timer.time / (float)TICK_TIMER;
		state[38] = galeform_gauge_timer.time / (float)TICK_TIMER;
		state[39] = inspire_gauge_timer.time / (float)TICK_TIMER;
		state[40] = inspire_sharp_timer.time / (float)(2.0f * TICK_TIMER);
		state[41] = divine_haste.count / 5.0f;
		state[42] = divine_haste.time / (float)DIVINE_HASTE_DURATION;
		state[43] = celestial_flier_cd.ready;
		state[44] = celestial_flier_cd.time / (float)CELESTIAL_FLIER_CD;
		state[45] = falcon_toss_procs > 0;
		state[46] = falcon_toss_procs > 1;
		state[47] = falcon_toss_timer.time / (float)FALCON_TOSS_CD;
		state[48] = muku_chief.count > 0;
		state[49] = muku_chief.time / (float)IMAGINE_DURATION;
		state[50] = celestial_flier.count > 0;
		state[51] = celestial_flier.time / (float)IMAGINE_DURATION;
		state[52] = chasing_str.count / 2.0f;
		state[53] = chasing_str.time / (float)CHASING_STR_DURATION;
		state[54] = tempestrike.count > 0;
		state[55] = tempestrike.time / (float)(3.0f * TEMPESTRIKE_BASE_DURATION);
		state[56] = typhoon_cleave.count > 0;
		state[57] = typhoon_cleave.time / (float)TYPHOON_CLEAVE_DURATION;
	}

	std::string BlackMage::get_info()
	{
		return "\n";
	}
}