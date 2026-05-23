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
		tempestrike_sharp_consumed = 0;
		falcon_gauge = 0.0f;
		fantasia_stacks = 0;

		in_air = false;
		azure = false;
		prev_falcon_toss = false;
		enhanced_galeform_next = false;
		enhanced_skyfall_next = false;
		enhanced_instant_edge_next = false;
		iec_next = false;

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
		tornado_timer_4.reset(0, false);
		tornado_timer_5.reset(0, false);
		tornado_timer_6.reset(0, false);
		falcon_toss_timer.reset(0, false);
		muku_chief_timer.reset(0, false);
		igoreus_timer.reset(0, false);

		galeform_procs = 2;
		falcon_toss_procs = MAX_FALCON_TOSS_PROCS;
		muku_chief_procs = 2;
		igoreus_procs = 2;

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
		tornado_4.reset(0, 0);
		tornado_5.reset(0, 0);
		tornado_6.reset(0, 0);
		muku_chief.reset(0, 0);
		celestial_flier.reset(0, 0);
		goblin_chief.reset(0, 0);
		goblin_king_armor_pen.reset(0, 0);
		goblin_king_luck.reset(0, 0);
		goblin_king_str.reset(0, 0);
		goblin_king_wind.reset(0, 0);
		goblin_king_boss_luck.reset(0, 0);
		igoreus.reset(0, 0);

		// cooldowns
		typhoon_cleave_cd.reset(0, true);
		spear_thrust_cd.reset(0, true);
		celestial_flier_cd.reset(0, true);
		goblin_chief_cd.reset(0, true);
		goblin_king_cd.reset(0, true);
		phantom_arrow_cd.reset(0, true);
		fantasia_impact_cd.reset(0, true);

		// actions
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = -1;
		cast_frame = -1;
		cast_speed = 1.0f;

		if (opener == Opener::GAUGE)
		{
			sharp = MAX_SHARP_STACKS;
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(SHARP_DURATION);
			impact = MAX_IMPACT;
		}

		// metrics
		tornado_count = 0;
		skyfall_count = 0;
		phantom_arrow_count = 0;
		fantasia_impact_count = 0;

		total_tornado_damage = 0.0f;
		total_phantom_arrow_damage = 0.0f;
		total_fantasia_damage = 0.0f;
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
		tempestrike_sharp_consumed = blm.tempestrike_sharp_consumed;
		falcon_gauge = blm.falcon_gauge;
		fantasia_stacks = blm.fantasia_stacks;

		in_air = blm.in_air;
		azure = blm.azure;
		prev_falcon_toss = blm.prev_falcon_toss;
		enhanced_galeform_next = blm.enhanced_galeform_next;
		enhanced_skyfall_next = blm.enhanced_skyfall_next;
		enhanced_instant_edge_next = blm.enhanced_instant_edge_next;
		iec_next = blm.iec_next;

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
		tornado_timer_4 = blm.tornado_timer_4;
		tornado_timer_5 = blm.tornado_timer_5;
		tornado_timer_6 = blm.tornado_timer_6;
		falcon_toss_timer = blm.falcon_toss_timer;
		muku_chief_timer = blm.muku_chief_timer;
		igoreus_timer = blm.igoreus_timer;

		galeform_procs = blm.galeform_procs;
		falcon_toss_procs = blm.falcon_toss_procs;
		muku_chief_procs = blm.muku_chief_procs;
		igoreus_procs = blm.igoreus_procs;

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
		tornado_4 = blm.tornado_4;
		tornado_5 = blm.tornado_5;
		tornado_6 = blm.tornado_6;
		muku_chief = blm.muku_chief;
		celestial_flier = blm.celestial_flier;
		goblin_chief = blm.goblin_chief;
		goblin_king_armor_pen = blm.goblin_king_armor_pen;
		goblin_king_luck = blm.goblin_king_luck;
		goblin_king_str = blm.goblin_king_str;
		goblin_king_wind = blm.goblin_king_wind;
		goblin_king_boss_luck = blm.goblin_king_boss_luck;
		igoreus = blm.igoreus;

		// cooldowns
		typhoon_cleave_cd = blm.typhoon_cleave_cd;
		spear_thrust_cd = blm.spear_thrust_cd;
		celestial_flier_cd = blm.celestial_flier_cd;
		goblin_chief_cd = blm.goblin_chief_cd;
		goblin_king_cd = blm.goblin_king_cd;
		phantom_arrow_cd = blm.phantom_arrow_cd;
		fantasia_impact_cd = blm.fantasia_impact_cd;

		// actions
		cast_timer = blm.cast_timer;
		action_timer = blm.action_timer;
		casting = blm.casting;
		cast_frame = blm.cast_frame;
		cast_speed = blm.cast_speed;

		// metrics
		tornado_count = 0;
		skyfall_count = 0;
		phantom_arrow_count = 0;
		fantasia_impact_count = 0;

		total_tornado_damage = 0.0f;
		total_phantom_arrow_damage = 0.0f;
		total_fantasia_damage = 0.0f;
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
		tornado_timer_4.update(elapsed);
		tornado_timer_5.update(elapsed);
		tornado_timer_6.update(elapsed);
		falcon_toss_timer.update(elapsed);
		muku_chief_timer.update(elapsed);
		igoreus_timer.update(elapsed);

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
		tornado_4.update(elapsed);
		tornado_5.update(elapsed);
		tornado_6.update(elapsed);
		muku_chief.update(elapsed);
		celestial_flier.update(elapsed);
		goblin_chief.update(elapsed);
		goblin_king_armor_pen.update(elapsed);
		goblin_king_luck.update(elapsed);
		goblin_king_str.update(elapsed);
		goblin_king_wind.update(elapsed);
		goblin_king_boss_luck.update(elapsed);
		igoreus.update(elapsed);

		// cooldowns
		typhoon_cleave_cd.update(elapsed);
		spear_thrust_cd.update(elapsed);
		celestial_flier_cd.update(elapsed);
		goblin_chief_cd.update(elapsed);
		goblin_king_cd.update(elapsed);
		phantom_arrow_cd.update(elapsed);
		fantasia_impact_cd.update(elapsed);

		// actions
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		//
		update_gauge();
		if (galeform_active && galeform.count == 0)
		{
			galeform_active = false;
			int extra_secs = tempestrike_gauge / 50.0f + tempestrike_sharp_consumed / 3;
			tempestrike_gauge = 0.0f;
			tempestrike_sharp_consumed = 0;
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
		if (igoreus_timer.ready)
		{
			igoreus_procs++;
			assert(igoreus_timer.time == 0);
			assert(igoreus_procs <= 2);
			igoreus_timer.ready = false;
			if (igoreus_procs < 2)
			{
				igoreus_timer.time = IGOREUS_CD;
				push_event(IGOREUS_CD);
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
			t.fake_reward = 0.0f;
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
			sharp = std::min(sharp + 1, MAX_SHARP_STACKS);
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
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_1.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_1.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_1.reset(0, false);
		}
		if (tornado_timer_2.ready)
		{
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_2.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_2.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_2.reset(0, false);
		}
		if (tornado_timer_3.ready)
		{
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_3.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_3.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_3.reset(0, false);
		}
		if (tornado_timer_4.ready)
		{
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_4.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_4.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_4.reset(0, false);
		}
		if (tornado_timer_5.ready)
		{
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_5.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_5.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_5.reset(0, false);
		}
		if (tornado_timer_6.ready)
		{
			if (prob(rng) < TORNADO_CHANCE_TO_HIT)
			{
				float damage = get_damage(TORNADO_HIT);
				total_damage += damage;
				history.back().reward += damage;
				total_tornado_damage += damage;
			}
			if (tornado_6.time >= TORNADO_TICK_TIMER)
			{
				tornado_timer_6.reset(TORNADO_TICK_TIMER, false);
				push_event(TORNADO_TICK_TIMER);
			}
			else
				tornado_timer_6.reset(0, false);
		}
		if (push_tick)
			push_event(TICK_TIMER);
	}

	float BlackMage::get_cast_speed() const
	{
		float haste = (stats.haste / (stats.haste + STAT_MOD)) + divine_haste.count * DIVINE_HASTE + inspire.count * INSPIRE_HASTE + celestial_flier.count * CELESTIAL_FLIER_HASTE_PERCENT + OCEAN_HASTE;
		return 1.0f + stats.base_atk_spd + haste * 1.6f + (windfury.count > 0 ? WINDFURY_ASPD : 0.0f);
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
			return ENABLE_MUKU_CHIEF && !in_air && muku_chief_procs > 0;
		case CELESTIAL_FLIER:
			return ENABLE_CELESTIAL_FLIER && !in_air && celestial_flier_cd.ready;
		case GOBLIN_CHIEF:
			return ENABLE_GOBLIN_CHIEF && !in_air && goblin_chief_cd.ready;
		case GOBLIN_KING:
			return ENABLE_GOBLIN_KING && !in_air && goblin_king_cd.ready;
		case IGOREUS:
			return ENABLE_IGOREUS && !in_air && igoreus_procs > 0;
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
			if (!phantom_arrow_cd.ready)
			{
				int reduction = prob(rng) < PHANTOM_ARROW_DOUBLE_CD_REDUCTION_CHANCE ? PHANTOM_ARROW_DOUBLE_CD_REDUCTION : PHANTOM_ARROW_CD_REDUCTION;
				if (phantom_arrow_cd.time <= reduction)
					phantom_arrow_cd.reset(0, true);
				else
				{
					phantom_arrow_cd.reset(phantom_arrow_cd.time - reduction, false);
					push_event(phantom_arrow_cd.time);
				}
			}
			in_air = true;
			skyfall_count++;
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
			start_action(action, 0, TYPHOON_CLEAVE_ANIM, true, (ENABLE_BATTLE_CRY_TALENT ? 0 : BATTLE_CRY_ANIM) + (in_air ? FALL_ANIM : 0));
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
			if (galeform_active)
				tempestrike_sharp_consumed += INSTANT_EDGE_STACK_COST;
			gauge = std::min(gauge + GAUGE_PER_SHARP * INSTANT_EDGE_STACK_COST, MAX_GAUGE);
			impact = std::min(impact + INSTANT_EDGE_STACK_COST, MAX_IMPACT);
			in_air = false;
			if (ENABLE_PHANTOM_ARROW && phantom_arrow_cd.ready)
			{
				float damage = get_damage(PHANTOM_ARROW);
				total_damage += damage;
				history.back().reward += damage;
				total_phantom_arrow_damage += damage;
				phantom_arrow_count++;
				phantom_arrow_cd.reset(PHANTOM_ARROW_CD, false);
				push_event(phantom_arrow_cd.time);
			}
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
			sharp = std::min(sharp + 1, MAX_SHARP_STACKS);
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			in_air = true;
			prev_falcon_toss = true;
			falcon_crit = false;
			falcon_luck = false;
			if (ENABLE_PHANTOM_ARROW && phantom_arrow_cd.ready)
			{
				float damage = get_damage(PHANTOM_ARROW);
				total_damage += damage;
				history.back().reward += damage;
				total_phantom_arrow_damage += damage;
				phantom_arrow_count++;
				phantom_arrow_cd.reset(PHANTOM_ARROW_CD, false);
				push_event(phantom_arrow_cd.time);
			}
			return;
		case AZURE_SEVER:
			if (!in_air)
				start_action(action, 0, FALCON_TOSS_ANIM_1, true);
			else
				start_action(action, 1, FALCON_TOSS_ANIM_2, true);
			sharp = std::min(sharp + 3, MAX_SHARP_STACKS);
			sharp_timer.reset(SHARP_DURATION, false);
			push_event(sharp_timer.time);
			azure = false;
			in_air = true;
			prev_falcon_toss = true;
			if (ENABLE_PHANTOM_ARROW && phantom_arrow_cd.ready)
			{
				float damage = get_damage(PHANTOM_ARROW);
				total_damage += damage;
				history.back().reward += damage;
				total_phantom_arrow_damage += damage;
				phantom_arrow_count++;
				phantom_arrow_cd.reset(PHANTOM_ARROW_CD, false);
				push_event(phantom_arrow_cd.time);
			}
			return;
		case SHARP_IMPACT:
			if (!in_air)
				start_action(action, 0, SHARP_IMPACT_GROUND_ANIM, true, SHARP_IMPACT_GROUND_FIXED_JUMP_ANIM);
			else
				start_action(action, 0, SHARP_IMPACT_AIR_ANIM, true);
			sharp = std::min(sharp + SHARP_IMPACT_SHARP, MAX_SHARP_STACKS);
			sharp_timer.reset(SHARP_DURATION, false);
			windfury.reset(WINDFURY_DURATION, 1);
			azure = true;
			impact = 0;
			enhanced_galeform_next = true;
			in_air = false;
			push_event(sharp_timer.time);
			push_event(windfury.time);
			if (ENABLE_PHANTOM_ARROW && phantom_arrow_cd.ready)
			{
				float damage = get_damage(PHANTOM_ARROW);
				total_damage += damage;
				history.back().reward += damage;
				total_phantom_arrow_damage += damage;
				phantom_arrow_count++;
				phantom_arrow_cd.reset(PHANTOM_ARROW_CD, false);
				push_event(phantom_arrow_cd.time);
			}
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
			if (ENABLE_PHANTOM_ARROW && phantom_arrow_cd.ready)
			{
				float damage = get_damage(PHANTOM_ARROW);
				total_damage += damage;
				history.back().reward += damage;
				total_phantom_arrow_damage += damage;
				phantom_arrow_count++;
				phantom_arrow_cd.reset(PHANTOM_ARROW_CD, false);
				push_event(phantom_arrow_cd.time);
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
		case GOBLIN_CHIEF:
			cast_speed = 1.0f;
			start_action(action, 0, IMAGINE_FIXED_ANIM, false);
			goblin_chief_cd.reset(GOBLIN_CHIEF_CD, false);
			push_event(goblin_chief_cd.time);
			break;
		case GOBLIN_KING:
			// unaffected by cast speed
			cast_speed = 1.0f;
			start_action(action, 0, IMAGINE_FIXED_ANIM, false);
			goblin_king_cd.reset(GOBLIN_KING_CD, false);
			push_event(goblin_king_cd.time);
			break;
		case IGOREUS:
			// unaffected by cast speed
			cast_speed = 1.0f;
			start_action(action, 0, IMAGINE_FIXED_ANIM, false);
			igoreus_procs--;
			if (igoreus_timer.time == 0)
			{
				assert(!igoreus_timer.ready);
				igoreus_timer.reset(IGOREUS_CD, false);
				push_event(igoreus_timer.time);
			}
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
				if (ENABLE_SKYFALL_TALENT && prob(rng) < SKYFALL_TALENT_CHANCE)
					sharp = std::min(sharp + 2, MAX_SHARP_STACKS);
				else
					sharp = std::min(sharp + 1, MAX_SHARP_STACKS);
				sharp_timer.reset(SHARP_DURATION, false);
				push_event(sharp_timer.time);
				float crit = (stats.crit + MUKU_CHIEF_CRIT * muku_chief.count + IGOREUS_CRIT * igoreus.count) * (ENABLE_CRIT_MASTERY_FACTOR ? CRIT_FACTOR_RATE : 1.0f);
				float crit_rate = std::min(0.05f + crit / (crit + STAT_MOD) + IGOREUS_CRIT_BONUS * igoreus.count + OCEAN_CRIT, 1.0f);
				float luck = stats.luck + GOBLIN_KING_LUCK_BONUS * goblin_king_luck.count + GOBLIN_KING_BOSS_LUCK_BONUS * goblin_king_boss_luck.count;
				float luck_rate = 0.05f + luck / (luck + STAT_MOD) + (ENABLE_FANTASIA_IMPACT ? FANTASIA_BASE_LUCK : 0.0f) + OCEAN_LUCK;

				bool crit_hit = prob(rng) < crit_rate;
				bool luck_hit = prob(rng) < luck_rate;
				int stacks_to_add = (crit_hit ? 1 : 0) + (luck_hit ? 1 : 0);
				if (stacks_to_add > 0)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + stacks_to_add, 2));
					push_event(chasing_step.time);
				}

				if (ENABLE_SPEAR_THRUST_TALENT && spear_thrust_cd.ready && prob(rng) < luck_rate)
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
			sharp = std::min(sharp + 1, MAX_SHARP_STACKS);
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
				iec_next = true;
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
					sharp = std::min(sharp + 1, MAX_SHARP_STACKS);
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
				float crit = (stats.crit + MUKU_CHIEF_CRIT * muku_chief.count + IGOREUS_CRIT * igoreus.count) * (ENABLE_CRIT_MASTERY_FACTOR ? CRIT_FACTOR_RATE : 1.0f);
				float crit_rate = std::min(0.05f + crit / (crit + STAT_MOD) + IGOREUS_CRIT_BONUS * igoreus.count + OCEAN_CRIT, 1.0f);
				if (prob(rng) < crit_rate)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + 1, 2));
					push_event(chasing_step.time);
					falcon_crit = true;
				}
			}
			if (!falcon_luck)
			{
				float luck = stats.luck + GOBLIN_KING_LUCK_BONUS * goblin_king_luck.count + GOBLIN_KING_BOSS_LUCK_BONUS * goblin_king_boss_luck.count;
				float luck_rate = 0.05f + luck / (luck + STAT_MOD) + (ENABLE_FANTASIA_IMPACT ? FANTASIA_BASE_LUCK : 0.0f) + OCEAN_LUCK;
				if (prob(rng) < luck_rate)
				{
					chasing_step.reset(CHASE_DURATION, std::min(chasing_step.count + 1, 2));
					push_event(chasing_step.time);
					falcon_luck = true;
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
			if (galeform_active)
			{
				int extra_secs = tempestrike_gauge / 50.0f + tempestrike_sharp_consumed / 3;
				tempestrike_gauge = 0.0f;
				tempestrike_sharp_consumed = 0;
				tempestrike.reset(TEMPESTRIKE_BASE_DURATION + 1000 * extra_secs, 1);
				push_event(tempestrike.time);
				gauge = std::min(gauge + (galeform.time - 1) / 1000 * (galeform.count > 1 ? ENHANCED_GALEFORM_GAUGE_PER_TICK : GALEFORM_GAUGE_PER_TICK), MAX_GAUGE);
			}
			galeform_active = true;
			galeform.reset(GALEFORM_DURATION, enhanced_galeform_next ? 2 : 1);
			push_event(galeform.time);
			if (enhanced_galeform_next)
				gauge = std::min(gauge + ENHANCED_GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			else
				gauge = std::min(gauge + GALEFORM_GAUGE_PER_TICK, MAX_GAUGE);
			enhanced_galeform_next = false;
			if (ENABLE_SET_BONUS)
			{
				enhanced_skyfall_next = true;
				enhanced_instant_edge_next = true;
			}
			galeform_gauge_timer.reset(TICK_TIMER, false);
			push_event(galeform_gauge_timer.time);
			action_timer.reset(ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case MUKU_CHIEF:
			damage = get_damage(casting);
			muku_chief.reset(IMAGINE_DURATION, 1);
			push_event(muku_chief.time);
			action_timer.reset(IMAGINE_ANIM_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case CELESTIAL_FLIER:
			damage = get_damage(casting);
			celestial_flier.reset(IMAGINE_DURATION, 1);
			push_event(celestial_flier.time);
			action_timer.reset(IMAGINE_ANIM_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case GOBLIN_CHIEF:
			damage = get_damage(casting);
			goblin_chief.reset(IMAGINE_DURATION, 1);
			push_event(goblin_chief.time);
			action_timer.reset(IMAGINE_ANIM_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			break;
		case GOBLIN_KING:
			if (prob(rng) < 0.5f)
			{
				goblin_king_armor_pen.reset(GOBLIN_KING_ARMOR_PEN_DURATION, 1);
				push_event(goblin_king_armor_pen.time);
			}
			else
				goblin_king_luck.reset(IMAGINE_DURATION, 1);
			goblin_king_str.reset(IMAGINE_DURATION, 1);
			if (prob(rng) < GOBLIN_KING_BOSS_CHANCE)
				goblin_king_boss_luck.reset(IMAGINE_DURATION, 1);
			else
				goblin_king_wind.reset(IMAGINE_DURATION, 1);
			if (goblin_king_armor_pen.count)
				damage += get_damage(casting, 0);
			else
				damage += get_damage(casting, 1);
			damage += get_damage(casting, 2);
			if (goblin_king_wind.count)
				damage += get_damage(casting, 3);
			else
				damage += get_damage(casting, 4);
			push_event(goblin_king_str.time);
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
		if (tornado_1.count && tornado_2.count && tornado_3.count && tornado_4.count && tornado_5.count && tornado_6.count)
			throw 67;
		if (!tornado_1.count)
		{
			tornado_1.reset(TORNADO_DURATION, 1);
			tornado_timer_1.reset(TORNADO_TICK_TIMER, false);
		}
		else if (!tornado_2.count)
		{
			tornado_2.reset(TORNADO_DURATION, 1);
			tornado_timer_2.reset(TORNADO_TICK_TIMER, false);
		}
		else if (!tornado_3.count)
		{
			tornado_3.reset(TORNADO_DURATION, 1);
			tornado_timer_3.reset(TORNADO_TICK_TIMER, false);
		}
		else if (!tornado_4.count)
		{
			tornado_4.reset(TORNADO_DURATION, 1);
			tornado_timer_4.reset(TORNADO_TICK_TIMER, false);
		}
		else if (!tornado_5.count)
		{
			tornado_5.reset(TORNADO_DURATION, 1);
			tornado_timer_5.reset(TORNADO_TICK_TIMER, false);
		}
		else
		{
			tornado_6.reset(TORNADO_DURATION, 1);
			tornado_timer_6.reset(TORNADO_TICK_TIMER, false);
		}
		if (prob(rng) < TORNADO_CHANCE_TO_HIT)
		{
			float damage = get_damage(TORNADO_HIT);
			total_damage += damage;
			history.back().reward += damage;
			total_tornado_damage += damage;
		}
		push_event(TORNADO_DURATION);
		push_event(TORNADO_TICK_TIMER);
		tornado_count++;
	}

	float BlackMage::get_damage(int action, int hit)
	{
		float potency = 0.0f;
		float skill_atk = 0.0f;
		float dmg = 1.0f;
		float gen_dmg = stats.base_dmg + VULN_DMG;
		float ele_dmg = 1.0f;
		float dream_dmg = 1.0f;
		float mastery = stats.mastery * (ENABLE_CRIT_MASTERY_FACTOR ? MASTERY_FACTOR_RATE : 1.0f);
		float mastery_per = (0.06f + (mastery / (mastery + STAT_MOD)));
		float luck = stats.luck + GOBLIN_KING_LUCK_BONUS * goblin_king_luck.count + GOBLIN_KING_BOSS_LUCK_BONUS * goblin_king_boss_luck.count;
		float luck_rate = 0.05f + luck / (luck + STAT_MOD) + (ENABLE_FANTASIA_IMPACT ? FANTASIA_BASE_LUCK : 0.0f) + OCEAN_LUCK;
		float luck_multi = (0.40f + stats.base_luck_multi + 0.25f * luck_rate + GOBLIN_KING_LUCK_MULTI * goblin_king_luck.count + FANTASIA_LUCK_MULTI) * LUCKY_STRIKE_MULTIPLIER;
		float gen_luck_dmg = luck_rate + (goblin_king_boss_luck.count > 0 ? GOBLIN_KING_BOSS_LUCK_DMG : 0.0f);
		bool expertise = false;
		int roll_luck = 0;
		bool double_skill_crit = false;
		bool luck_effect = false;
		switch (action)
		{
		case BASIC_ATTACK:
			potency = BASIC_ATTACK_POTENCY;
			skill_atk = BASIC_ATTACK_ATK;
			roll_luck = 1;
			break;
		case SKYFALL:
			potency = SKYFALL_POTENCY;
			skill_atk = SKYFALL_ATK;
			dmg += galeform.count > 0 ? GALEFORM_BONUS_DMG : 0.0f;
			if (enhanced_skyfall_next)
			{
				ele_dmg += SET_BONUS_WIND_DMG;
				enhanced_skyfall_next = false;
			}
			if (ENABLE_SHARP_ECHO_TALENT)
				dmg += OTHER_SKYFALL_TALENT_DMG;
			roll_luck = 1;
			break;
		case TYPHOON_CLEAVE:
			potency = TYPHOON_CLEAVE_POTENCY;
			skill_atk = TYPHOON_CLEAVE_ATK;
			roll_luck = 1;
			break;
		case INSTANT_EDGE:
			if (enhanced_instant_edge_next)
				ele_dmg += SET_BONUS_WIND_DMG;
			if (hit == 0)
			{
				potency = INSTANT_EDGE_POTENCY_1;
				skill_atk = INSTANT_EDGE_ATK_1;
			}
			else
			{
				potency = INSTANT_EDGE_POTENCY_2;
				skill_atk = INSTANT_EDGE_ATK_2;
				enhanced_instant_edge_next = false;
			}
			dmg += INSTANT_EDGE_BREAK_DMG;
			expertise = true;
			dmg += galeform.count > 0 ? GALEFORM_BONUS_DMG : 0.0f;
			if (ENABLE_INSTANT_CRIT_TALENT && chasing_step.count > 0)
				double_skill_crit = true;
			if (chasing_step.count > 0)
				dmg += CHASING_STEP_DMG;
			roll_luck = 1;
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
				roll_luck = 1;
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
				roll_luck = 1;
			}
			expertise = true;
			break;
		case SHARP_IMPACT:
			potency = SHARP_IMPACT_POTENCY;
			skill_atk = SHARP_IMPACT_ATK;
			expertise = true;
			roll_luck = 1;
			if (ENABLE_INSTANT_CRIT_TALENT && chasing_step.count > 0)
				double_skill_crit = true;
			break;
		case GALEFORM:
			potency = GALEFORM_POTENCY;
			skill_atk = GALEFORM_ATK;
			expertise = true;
			roll_luck = 1;
			break;
		case SPEAR_THRUST:
			potency = SPEAR_THRUST_POTENCY;
			dmg += SPEAR_THRUST_BONUS_DMG;
			luck_effect = true;
			roll_luck = 1;
			break;
		case TORNADO_HIT:
			potency = TORNADO_POTENCY;
			dmg += TORNADO_BONUS_DMG;
			roll_luck = ENABLE_LUCK_TORNADO_FACTOR;
			if (ENABLE_LUCK_TORNADO_FACTOR)
				dream_dmg += LUCK_TORNADO_FACTOR_DREAM_DMG;
			break;
		case PHANTOM_ARROW:
			potency = PHANTOM_ARROW_POTENCY;
			dream_dmg += PHANTOM_ARROW_DREAM_DMG;
			dmg += mastery_per * PHANTOM_ARROW_MASTERY_DMG_CONVERSION;
			break;
		case FANTASIA_IMPACT:
			potency = FANTASIA_IMPACT_POTENCY;
			luck_effect = true;
			roll_luck = 1;
			break;
		case CELESTIAL_FLIER:
		case GOBLIN_CHIEF:
		case IGOREUS:
			potency = CELESTIAL_FLIER_POTENCY;
			skill_atk = CELESTIAL_FLIER_ATK;
			break;
		case MUKU_CHIEF:
			potency = MUKU_CHIEF_POTENCY;
			skill_atk = MUKU_CHIEF_ATK;
			break;
		case GOBLIN_KING:
			switch (hit)
			{
			case 0:
				potency = GOBLIN_KING_ARMOR_POTENCY;
				skill_atk = GOBLIN_KING_ARMOR_ATK;
				break;
			case 1:
				potency = GOBLIN_KING_LUCK_POTENCY;
				skill_atk = GOBLIN_KING_LUCK_ATK;
				break;
			case 2:
				potency = GOBLIN_KING_STR_POTENCY;
				skill_atk = GOBLIN_KING_STR_ATK;
				break;
			case 3:
				potency = GOBLIN_KING_WIND_POTENCY;
				skill_atk = GOBLIN_KING_WIND_ATK;
				break;
			default:
				potency = GOBLIN_KING_BOSS_POTENCY;
				skill_atk = GOBLIN_KING_BOSS_ATK;
			}
			break;
		case BATTLE_CRY:
		case FALL:
		case WAIT_FOR_GAUGE:
			throw 123;
		}
		if (windfury.count)
			gen_dmg += WINDFURY_BONUS_DMG;
		if (expertise)
		{
			dmg += EXP_SKILL_DMG + (ENABLE_GOBLIN_CHIEF ? GOBLIN_CHIEF_EXP_SKILL_DMG_PASSIVE : 0.0f) + (goblin_chief.count > 0 ? GOBLIN_CHIEF_EXP_SKILL_DMG_ACTIVE : 0.0f);
			dream_dmg += EXPERTISE_DREAM_DMG;
		}

		float str = stats.str + (galeform.count > 0 ? GALEFORM_FLAT_STR : 0.0f) + (ENABLE_FANTASIA_IMPACT ? FANTASIA_STAT_PER : 0.0f);
		float str_percent = 1.0f + tempestrike.count * TEMPESTRIKE_STR + chasing_str.count * CHASING_STR + (galeform.count > 0 ? GALEFORM_STR : 0.0f) + stats.base_str_per + (goblin_king_str.count > 0 ? GOBLIN_KING_STR_BONUS : 0.0f);
		str *= str_percent;

		float atk_percent = 1.0f + (sharp > 0 ? SHARP_ATK : 0.0f) + stats.base_atk_per;
		float total_atk = (stats.flat_atk + str * 0.725f) * atk_percent;
		float armor_pen = stats.base_armor_pen;
		float flat_armor_pen = goblin_king_armor_pen.count > 0 ? GOBLIN_KING_ARMOR_PEN : 0;
		float base_armor = (2786.0f - flat_armor_pen) * (1.0f - armor_pen);
		float armor = base_armor / (base_armor + 6500.0f);
		if (action == PHANTOM_ARROW)
			armor = 0.0f;
		float atk = (1.0f - armor) * total_atk + stats.refined_atk;

		if (luck_effect)
			dmg += gen_luck_dmg;

		dmg += gen_dmg;

		float ele_stat = stats.base_ele_stat + (ENABLE_ALL_ELEMENT_FACTOR ? ALL_ELEMENT_FACTOR_BONUS : 0.0f);
		float base_ele_dmg = mastery_per * 0.65f + (ele_stat / (ele_stat + ELE_MOD)) + (stats.base_serum_stat / (stats.base_serum_stat + ELE_MOD)) + (goblin_king_wind.count > 0 ? GOBLIN_KING_WIND_BONUS : 0.0f);
		ele_dmg += base_ele_dmg;

		float vers_dmg = 1.0f + (stats.vers / (stats.vers + VERS_MOD)) * 0.35f;

		float crit = (stats.crit + MUKU_CHIEF_CRIT * muku_chief.count + IGOREUS_CRIT * igoreus.count) * (ENABLE_CRIT_MASTERY_FACTOR ? CRIT_FACTOR_RATE : 1.0f);
		float uncapped_crit_rate = 0.05f + crit / (crit + STAT_MOD) + IGOREUS_CRIT_BONUS * igoreus.count + OCEAN_CRIT;
		float crit_rate = std::min(uncapped_crit_rate, 1.0f);
		float eff_crit_rate = std::min((double_skill_crit ? 2.0f : 1.0f) * crit_rate, 1.0f);
		if ((action == PHANTOM_ARROW && !ENABLE_PHANTOM_ARROW_CAN_CRIT) || action == FANTASIA_IMPACT)
			eff_crit_rate = 0.0f;
		float crit_multi = 0.50f + stats.base_crit_multi + MUKU_CHIEF_CRIT_MULTI * muku_chief.count + (inspire.count > 0 ? BATTLE_CRY_CRIT_DMG_BONUS : 0.0f) + (ENABLE_IGOREUS ? IGOREUS_PASSIVE_CRIT_DMG : 0.0f);

		if (double_skill_crit)
		{
			uncapped_crit_rate *= 2.0f;
			float overflow = std::max(0.0f, uncapped_crit_rate - 1.0f);
			crit_multi += overflow * 3.0f;
		}

		if (ENABLE_IGOREUS)
		{
			float excess_crit_rate = std::max(0.0f, uncapped_crit_rate - 0.60f);
			crit_multi += std::min(IGOREUS_CRIT_DMG_LIMIT, excess_crit_rate * IGOREUS_EXCESS_CRIT_CONV);
		}

		// * dmg * ele_dmg * vers_dmg * dream_dmg
		float skill_dmg = (atk * potency + skill_atk) * dmg * ele_dmg * vers_dmg * dream_dmg * (galeform.count > 0 ? OCEAN_GALEFORM_BOOST : 1.0f);
		if (luck_effect)
			skill_dmg *= luck_multi;
		float bonus_dmg = 0.0f;
		if (ENABLE_INSTANT_EDGE_COMBO_TALENT && action == INSTANT_EDGE && hit == 1)
		{
			float iec_luck_rate = iec_next ? (1.0f - (1.0f - luck_rate) * (1.0f - luck_rate)) : luck_rate;
			if (prob(rng) < iec_luck_rate)
				roll_luck++;
			bonus_dmg = skill_dmg * (1.0f + eff_crit_rate * crit_multi) * luck_rate * luck_multi * INSTANT_EDGE_COMBO_DREAM_DMG;
			iec_next = false;
		}

		if (ENABLE_EXP_CRIT_PEN_TALENT && expertise)
		{
			skill_dmg *= 1.0f - eff_crit_rate;
			armor_pen += 0.50f;
			base_armor = (2786.0f - flat_armor_pen) * (1.0f - armor_pen);
			armor = base_armor / (base_armor + 6500.0f);
			atk = (1.0f - armor) * total_atk + stats.refined_atk;
			skill_dmg += (atk * potency + skill_atk) * dmg * ele_dmg * vers_dmg * dream_dmg * eff_crit_rate * (1.0f + crit_multi) * (galeform.count > 0 ? OCEAN_GALEFORM_BOOST : 1.0f);
		}
		else
			// skill crit dmg
			skill_dmg *= 1.0f + eff_crit_rate * crit_multi;

		skill_dmg += bonus_dmg;

		// lucky strike dmg
		for (int i = 0; i < roll_luck; i++)
		{
			skill_dmg += luck_rate * (total_atk + stats.refined_atk) * (1.0f + gen_dmg + gen_luck_dmg) * (1.0f + base_ele_dmg) * vers_dmg * luck_multi * (1.0f + crit_rate * crit_multi) * (galeform.count > 0 ? OCEAN_GALEFORM_BOOST : 1.0f);
			// add side effect
			if (ENABLE_SET_BONUS && prob(rng) < (action == TORNADO_HIT ? luck_rate * 1.55f : luck_rate))
			{
				if (ENABLE_SET_BONUS && galeform_procs < 2)
				{
					if (galeform_timer.time <= SET_GALEFORM_CD_REDUCTION)
					{
						galeform_procs++;
						if (galeform_procs == 2)
							galeform_timer.reset(0, false);
						else
						{
							galeform_timer.reset(GALEFORM_CD + galeform_timer.time - SET_GALEFORM_CD_REDUCTION, false);
							push_event(galeform_timer.time);
						}
					}
					else
					{
						galeform_timer.reset(galeform_timer.time - SET_GALEFORM_CD_REDUCTION, false);
						push_event(galeform_timer.time);
					}
				}
				if (ENABLE_FANTASIA_IMPACT)
				{
					if (!fantasia_impact_cd.ready)
					{
						if (fantasia_impact_cd.time <= FANTASIA_IMPACT_CD_REDUCTION)
							fantasia_impact_cd.reset(0, true);
						else
						{
							fantasia_impact_cd.reset(fantasia_impact_cd.time - FANTASIA_IMPACT_CD_REDUCTION, false);
							push_event(fantasia_impact_cd.time);
						}
					}
					else if (++fantasia_stacks == FANTASIA_IMPACT_MAX_STACKS)
					{
						fantasia_stacks = 0;
						fantasia_impact_cd.reset(FANTASIA_IMPACT_CD, false);
						push_event(fantasia_impact_cd.time);
						float fantasia_dmg = get_damage(FANTASIA_IMPACT);
						skill_dmg += fantasia_dmg;
						fantasia_impact_count++;
						total_fantasia_damage += fantasia_dmg;
					}
				}
			}
		}

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
		state[15] = (SHARP_DURATION - sharp_timer.time) / (float)SHARP_DURATION;
		state[16] = chasing_step.count > 0;
		state[17] = chasing_step.count > 1;
		state[18] = chasing_step.time / (float)CHASE_DURATION;
		state[19] = inspire.count > 0;
		state[20] = inspire.time / (float)INSPIRE_DURATION;
		state[21] = windfury.count > 0;
		state[22] = windfury.time / (float)WINDFURY_DURATION;
		state[23] = galeform.count > 0;
		state[24] = galeform.count > 1;
		state[25] = galeform.time / (float)GALEFORM_DURATION;
		state[26] = (tornado_1.time + tornado_2.time + tornado_3.time + tornado_4.time + tornado_5.time + tornado_6.time) / (5.0f * TORNADO_DURATION);
		state[27] = typhoon_cleave_cd.ready;
		state[28] = typhoon_cleave_cd.time / (float)TYPHOON_CLEAVE_CD;
		state[29] = gauge_timer.time / (float)TICK_TIMER;
		state[30] = galeform_gauge_timer.time / (float)TICK_TIMER;
		state[31] = inspire_gauge_timer.time / (float)TICK_TIMER;
		state[32] = inspire_sharp_timer.time / (float)(2.0f * TICK_TIMER);
		state[33] = divine_haste.count / 5.0f;
		state[34] = divine_haste.time / (float)DIVINE_HASTE_DURATION;
		state[35] = falcon_toss_procs > 0;
		state[36] = falcon_toss_procs > 1;
		state[37] = falcon_toss_timer.time / (float)FALCON_TOSS_CD;
		state[38] = chasing_str.count / 2.0f;
		state[39] = chasing_str.time / (float)CHASING_STR_DURATION;
		state[40] = tempestrike.count > 0;
		state[41] = tempestrike.time / (float)(4.0f * TEMPESTRIKE_BASE_DURATION);
		state[42] = typhoon_cleave.count > 0;
		state[43] = typhoon_cleave.time / (float)TYPHOON_CLEAVE_DURATION;
		state[44] = enhanced_skyfall_next;
		state[45] = enhanced_instant_edge_next;
		int idx = 46;
		if (ENABLE_PHANTOM_ARROW)
		{
			state[idx++] = phantom_arrow_cd.ready;
			state[idx++] = phantom_arrow_cd.time / (float)PHANTOM_ARROW_CD;
		}
		if (ENABLE_FANTASIA_IMPACT)
		{
			state[idx++] = fantasia_stacks / (float)FANTASIA_IMPACT_MAX_STACKS;
			state[idx++] = fantasia_impact_cd.ready;
			state[idx++] = fantasia_impact_cd.time / (float)FANTASIA_IMPACT_CD;
		}
		if (ENABLE_MUKU_CHIEF)
		{
			state[idx++] = muku_chief_procs > 0;
			state[idx++] = muku_chief_procs > 1;
			state[idx++] = muku_chief_timer.time / (float)MUKU_CHIEF_CD;
			state[idx++] = muku_chief.count > 0;
			state[idx++] = muku_chief.time / (float)IMAGINE_DURATION;
		}
		if (ENABLE_GOBLIN_KING)
		{
			state[idx++] = goblin_king_cd.ready;
			state[idx++] = goblin_king_cd.time / (float)GOBLIN_KING_CD;
			state[idx++] = goblin_king_str.count > 0;
			state[idx++] = goblin_king_str.time / (float)IMAGINE_DURATION;
		}
		if (ENABLE_CELESTIAL_FLIER)
		{
			state[idx++] = celestial_flier.count > 0;
			state[idx++] = celestial_flier.time / (float)IMAGINE_DURATION;
			state[idx++] = celestial_flier_cd.ready;
			state[idx++] = celestial_flier_cd.time / (float)CELESTIAL_FLIER_CD;
		}
		if (ENABLE_GOBLIN_CHIEF)
		{
			state[idx++] = goblin_chief_cd.ready;
			state[idx++] = goblin_chief_cd.time / (float)GOBLIN_CHIEF_CD;
			state[idx++] = goblin_chief.count > 0;
			state[idx++] = goblin_chief.time / (float)IMAGINE_DURATION;
		}
		if (ENABLE_IGOREUS)
		{
			state[idx++] = igoreus_procs > 0;
			state[idx++] = igoreus_procs > 1;
			state[idx++] = igoreus_timer.time / (float)IGOREUS_CD;
			state[idx++] = igoreus.count > 0;
			state[idx++] = igoreus.time / (float)IMAGINE_DURATION;
		}
	}

	std::string BlackMage::get_info()
	{
		return "\n";
	}
}
