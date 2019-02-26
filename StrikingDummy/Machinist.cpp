#include "Machinist.h"
#include <assert.h>

namespace StrikingDummy
{
	Machinist::Machinist(Stats& stats) :
		Job(stats, MCH_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD)))),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay))))
	{
		crit_expected_multiplier = this->stats.crit_multiplier * ((1.0f - this->stats.dhit_rate) + 1.25f * this->stats.dhit_rate);
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Machinist::reset()
	{
		timeline = {};

		heat = 0;
		ammo = 3;
		gauss = true;
		burning = false;

		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		turret_timer.reset(std::uniform_int_distribution<int>(1, TURRET_TIMER)(rng), false);
		timeline.push_event(auto_timer.time);
		timeline.push_event(turret_timer.time);

		// buffs
		slug_bonus.reset(0, 0);
		clean_bonus.reset(0, 0);
		reassemble.reset(0, 0);
		hot.reset(0, 0);
		rapidfire.reset(0, 0);
		wildfire.reset(0, 0);
		overheat.reset(0, 0);
		hypercharge.reset(0, 0);
		flamethrower.reset(0, 0);
		vuln.reset(0, 0);

		// cooldowns
		reload_cd.reset(0, true);
		reassemble_cd.reset(0, true);
		quick_reload_cd.reset(0, true);
		rapidfire_cd.reset(0, true);
		wildfire_cd.reset(0, true);
		barrel_cd.reset(0, false); // gauss barrel starts on cd
		gauss_round_cd.reset(0, true);
		hypercharge_cd.reset(0, true);
		ricochet_cd.reset(0, true);
		stabilizer_cd.reset(0, true);
		flamethrower_cd.reset(0, true);
		overdrive_cd.reset(0, true);
		turret_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = NONE;

		// metrics
		total_damage = 0.0f;

		history.clear();

		update_history();
	}

	void Machinist::update(int elapsed)
	{
		assert(elapsed > 0);

		// ticks
		auto_timer.update(elapsed);
		turret_timer.update(elapsed);

		// buffs
		slug_bonus.update(elapsed);
		clean_bonus.update(elapsed);
		reassemble.update(elapsed);
		hot.update(elapsed);
		rapidfire.update(elapsed);
		wildfire.update(elapsed);
		overheat.update(elapsed);
		hypercharge.update(elapsed);
		flamethrower.update(elapsed);
		vuln.update(elapsed);

		// cooldowns
		reload_cd.update(elapsed);
		reassemble_cd.update(elapsed);
		quick_reload_cd.update(elapsed);
		rapidfire_cd.update(elapsed);
		wildfire_cd.update(elapsed);
		barrel_cd.update(elapsed);
		gauss_round_cd.update(elapsed);
		hypercharge_cd.update(elapsed);
		ricochet_cd.update(elapsed);
		stabilizer_cd.update(elapsed);
		flamethrower_cd.update(elapsed);
		overdrive_cd.update(elapsed);
		turret_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		// 
		if (auto_timer.ready)
			update_auto();
		if (turret_timer.ready)
			update_turret();
		if (cast_timer.ready)
			end_action();
		if (burning && overheat.count == 0)
		{
			heat = 0;
			gauss = false;
			burning = false;
			barrel_cd.reset(BARREL_CD, false);
			push_event(BARREL_CD);
		}

		update_history();
	}

	void Machinist::update_history()
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
			t.dt = timeline.time;
		}
	}


	void Machinist::update_auto()
	{
		float damage = get_auto_damage();
		total_damage += damage;
		history.back().reward += damage;
		auto_timer.reset(auto_gcd, false);
		push_event(auto_timer.time);
	}

	void Machinist::update_turret()
	{
		if (turret_cd.ready)
		{
			float damage = get_turret_damage();
			total_damage += damage;
			history.back().reward += damage;
			if (hypercharge.count > 0)
			{
				vuln.reset(VULN_DURATION, 1);
				push_event(VULN_DURATION);
			}
		}
		turret_timer.reset(TURRET_TIMER, false);
		push_event(turret_timer.time);
	}

	int Machinist::get_gcd_time() const
	{
		return rapidfire.count > 0 ? rapid_gcd : base_gcd;
	}

	bool Machinist::can_use_action(int action) const
	{
		switch (action)
		{
		case NONE:
			return !gcd_timer.ready;
		case SPLIT:
			return gcd_timer.ready && slug_bonus.count == 0;
		case SLUG:
			return gcd_timer.ready && slug_bonus.count > 0 && clean_bonus.count == 0;
		case HOT:
			return gcd_timer.ready;
		case CLEAN:
			return gcd_timer.ready && clean_bonus.count > 0;
		case COOLDOWN:
			return gcd_timer.ready && heat >= 50;
		case GAUSS_ROUND:
			return gauss_round_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case RICOCHET:
			return ricochet_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case RELOAD:
			return reload_cd.ready && ammo < 3 && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX; // i.e., can reload if not full
		case REASSEMBLE:
			return reassemble_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case QUICK_RELOAD:
			return quick_reload_cd.ready && ammo < 3 && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case RAPIDFIRE:
			return rapidfire_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case WILDFIRE:
			return wildfire_cd.ready && (gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX || flamethrower.count > 0); // i.e., can cancel flamethrower with wildfire
		case GAUSS_BARREL:
			return barrel_cd.ready && !gauss && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case STABILIZER:
			return stabilizer_cd.ready && gauss && overheat.count == 0 && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case HYPERCHARGE:
			return hypercharge_cd.ready && turret_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case OVERDRIVE:
			return overdrive_cd.ready && turret_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case FLAMETHROWER_CAST:
			return flamethrower_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case FLAMETHROWER_TICK:
			return flamethrower.count > 0;
		}
		return false;
	}

	void Machinist::use_action(int action)
	{
		history.back().action = action;
		if (flamethrower.count > 0 && action != FLAMETHROWER_TICK)
			flamethrower.reset(0, 0);
		switch (action)
		{
		case NONE:
			action_timer.reset(gcd_timer.time, false);
			push_event(action_timer.time);
			return;
		case SPLIT:
		case SLUG:
		case HOT:
		case CLEAN:
		case COOLDOWN:
		case GAUSS_ROUND:
		case RICOCHET:
		case OVERDRIVE:
			use_damage_action(action);
			break;
		case FLAMETHROWER_CAST:
			casting = action;
			cast_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
			action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
			flamethrower.reset(FLAMETHROWER_DURATION, 1);
			flamethrower_cd.reset(FLAMETHROWER_CD, false);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			push_event(FLAMETHROWER_DURATION);
			push_event(FLAMETHROWER_CD);
			return;
		case FLAMETHROWER_TICK:
			casting = action;
			cast_timer.reset(FT_TICK, false);
			action_timer.reset(FT_TICK, false);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			return;
		case RELOAD:
			ammo = 3;
			reload_cd.reset(RELOAD_CD, false);
			push_event(RELOAD_CD);
			break;
		case REASSEMBLE:
			reassemble.reset(REASSEMBLE_DURATION, 1);
			reassemble_cd.reset(REASSEMBLE_CD, false);
			push_event(REASSEMBLE_DURATION);
			push_event(REASSEMBLE_CD);
			break;
		case QUICK_RELOAD:
			ammo++;
			quick_reload_cd.reset(QUICK_RELOAD_CD, false);
			push_event(QUICK_RELOAD_CD);
			break;
		case RAPIDFIRE:
			rapidfire.reset(RAPIDFIRE_DURATION, 3);
			rapidfire_cd.reset(RAPIDFIRE_CD, false);
			push_event(RAPIDFIRE_DURATION);
			push_event(RAPIDFIRE_CD);
			break;
		case WILDFIRE:
			wildfire.reset(WILDFIRE_DURATION, 1);
			wildfire_cd.reset(WILDFIRE_CD, false);
			push_event(WILDFIRE_DURATION);
			push_event(WILDFIRE_CD);
			break;
		case GAUSS_BARREL:
			gauss = true;
			barrel_cd.reset(0, false);
			break;
		case STABILIZER:
			heat = 50;
			stabilizer_cd.reset(STABILIZER_CD, false);
			push_event(STABILIZER_CD);
			break;
		case HYPERCHARGE:
			hypercharge.reset(HYPERCHARGE_DURATION, 1);
			hypercharge_cd.reset(HYPERCHARGE_CD, false);
			push_event(HYPERCHARGE_DURATION);
			push_event(HYPERCHARGE_CD);
			break;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
	}

	void Machinist::use_damage_action(int action)
	{
		float damage = get_damage(action);
		total_damage += damage;
		history.back().reward += damage;

		bool weaponskill = false;
		int gcd_time = get_gcd_time();

		switch (action)
		{
		case SPLIT:
			if (gauss && ammo == 0)
				heat = std::min(MAX_HEAT, heat + 5);
			if (prob(rng) < BONUS_PROBABILITY || ammo > 0)
			{
				slug_bonus.reset(BONUS_DURATION, 1);
				push_event(BONUS_DURATION);
			}
			weaponskill = true;
			break;
		case SLUG:
			if (gauss && ammo == 0)
				heat = std::min(MAX_HEAT, heat + 5);
			if (prob(rng) < BONUS_PROBABILITY || ammo > 0)
			{
				clean_bonus.reset(BONUS_DURATION, 1);
				push_event(BONUS_DURATION);
			}
			slug_bonus.reset(0, 0);
			weaponskill = true;
			break;
		case HOT:
			if (gauss && ammo == 0)
				heat = std::min(MAX_HEAT, heat + 5);
			hot.reset(HOT_DURATION, 1);
			push_event(HOT_DURATION);
			weaponskill = true;
			break;
		case CLEAN:
			if (gauss && ammo == 0)
				heat = std::min(MAX_HEAT, heat + 5);
			clean_bonus.reset(0, 0);
			weaponskill = true;
			break;
		case COOLDOWN:
			if (!burning)
				heat -= 25;
			weaponskill = true;
			break;
		case GAUSS_ROUND:
			gauss_round_cd.reset(GAUSS_ROUND_CD, false);
			push_event(GAUSS_ROUND_CD);
			break;
		case RICOCHET:
			ricochet_cd.reset(RICOCHET_CD, false);
			push_event(RICOCHET_CD);
			break;
		case OVERDRIVE:
			overdrive_cd.reset(OVERDRIVE_CD, false);
			turret_cd.reset(TURRET_CD, false);
			push_event(OVERDRIVE_CD);
			push_event(TURRET_CD);
			break;
		}
		if (weaponskill)
		{
			if (rapidfire.count > 0)
				if (--rapidfire.count == 0)
					rapidfire.reset(0, 0);
			if (reassemble.count > 0)
				reassemble.reset(0, 0);
			if (ammo > 0)
				ammo--;
			if (heat == MAX_HEAT && !burning)
			{
				burning = true;
				overheat.reset(OVERHEAT_DURATION, 1);
				push_event(OVERHEAT_DURATION);
			}
			gcd_timer.reset(gcd_time, false);
			push_event(gcd_time);
		}
	}

	void Machinist::end_action()
	{
		assert(casting != NONE);

		float damage = get_damage(casting);
		total_damage += damage;
		history.back().reward += damage;

		if (gauss)
			heat = std::min(MAX_HEAT, heat + 20);
		if (heat == MAX_HEAT && !burning)
		{
			burning = true;
			overheat.reset(OVERHEAT_DURATION, 1);
			push_event(OVERHEAT_DURATION);
		}

		casting = NONE;
		cast_timer.ready = false;
	}

	float Machinist::get_damage(int action) const
	{
		float potency = 0.0f;
		bool weaponskill = false;
		bool heated = heat >= 50;
		switch (action)
		{
		case SPLIT:
			potency = (heated ? HEATED_SPLIT_POTENCY : SPLIT_POTENCY) + (ammo > 0 ? AMMO_POTENCY : 0);
			weaponskill = true;
			break;
		case SLUG:
			potency = (heated ? HEATED_BONUS_SLUG_POTENCY : BONUS_SLUG_POTENCY) + (ammo > 0 ? AMMO_POTENCY : 0);
			weaponskill = true;
			break;
		case HOT:
			potency = HOT_POTENCY + (ammo > 0 ? AMMO_POTENCY : 0);
			weaponskill = true;
			break;
		case CLEAN:
			potency = (heated ? HEATED_BONUS_CLEAN_POTENCY : BONUS_CLEAN_POTENCY) + (ammo > 0 ? AMMO_POTENCY : 0);
			weaponskill = true;
			break;
		case COOLDOWN:
			potency = HEATED_COOLDOWN_POTENCY + (ammo > 0 ? AMMO_POTENCY : 0);
			weaponskill = true;
			break;
		case GAUSS_ROUND:
			potency = GAUSS_ROUND_POTENCY;
			break;
		case RICOCHET:
			potency = RICOCHET_POTENCY;
			break;
		case OVERDRIVE:
			return OVERDRIVE_POTENCY * stats.potency_multiplier * ACTION_MULTIPLIER *
				(vuln.count > 0 ? VULN_MULTIPLIER : 1.0f) *
				stats.expected_multiplier;
		case FLAMETHROWER_CAST:
		case FLAMETHROWER_TICK:
			potency = FLAMETHROWER_POTENCY;
			break;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		float damage = potency * stats.potency_multiplier * ACTION_MULTIPLIER *
			(hot.count > 0 ? HOT_MULTIPLIER : 1.0f) *
			(vuln.count > 0 ? VULN_MULTIPLIER : 1.0f) *
			(wildfire.count > 0 ? WILDFIRE_MULTIPLIER : 1.0f) *
			(gauss ? GAUSS_MULTIPLIER : 1.0f) *
			(burning ? OVERHEAT_MULTIPLIER : 1.0f);
		if (reassemble.count > 0 && weaponskill)
			return damage * crit_expected_multiplier;
		return damage * stats.expected_multiplier;
	}

	float Machinist::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier * ACTION_MULTIPLIER *
			(hot.count > 0 ? HOT_MULTIPLIER : 1.0f) *
			(vuln.count > 0 ? VULN_MULTIPLIER : 1.0f) *
			(wildfire.count > 0 ? WILDFIRE_MULTIPLIER : 1.0f) *
			(gauss ? GAUSS_MULTIPLIER : 1.0f) *
			(burning ? OVERHEAT_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	float Machinist::get_turret_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return (hypercharge.count > 0 ? HYPERCHARGE_POTENCY : TURRET_POTENCY) * stats.potency_multiplier * ACTION_MULTIPLIER *
			(vuln.count > 0 ? VULN_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	void Machinist::get_state(float* state)
	{
		state[0] = heat / (float)MAX_HEAT;
		state[1] = ammo >= 1;
		state[2] = ammo >= 2;
		state[3] = ammo >= 3;
		state[4] = gauss;
		state[5] = burning;
		state[6] = slug_bonus.count;
		state[7] = slug_bonus.time / (float)BONUS_DURATION;
		state[8] = clean_bonus.count;
		state[9] = clean_bonus.time / (float)BONUS_DURATION;
		state[10] = reassemble.count;
		state[11] = reassemble.time / (float)REASSEMBLE_DURATION;
		state[12] = hot.count;
		state[13] = hot.time / (float)HOT_DURATION;
		state[14] = rapidfire.count >= 1;
		state[15] = rapidfire.count >= 2;
		state[16] = rapidfire.count >= 3;
		state[17] = rapidfire.time / (float)RAPIDFIRE_DURATION;
		state[18] = wildfire.count;
		state[19] = wildfire.time / (float)WILDFIRE_DURATION;
		state[20] = overheat.count;
		state[21] = overheat.time / (float)OVERHEAT_DURATION;
		state[22] = hypercharge.count;
		state[23] = hypercharge.time / (float)HYPERCHARGE_DURATION;
		state[24] = flamethrower.count;
		state[25] = flamethrower.time / (float)FLAMETHROWER_DURATION;
		state[26] = vuln.count;
		state[27] = vuln.time / (float)VULN_DURATION;
		state[28] = reload_cd.ready;
		state[29] = reload_cd.time / (float)RELOAD_CD;
		state[30] = reassemble_cd.ready;
		state[31] = reassemble_cd.time / (float)REASSEMBLE_CD;
		state[32] = quick_reload_cd.ready;
		state[33] = quick_reload_cd.time / (float)QUICK_RELOAD_CD;
		state[34] = rapidfire_cd.ready;
		state[35] = rapidfire_cd.time / (float)RAPIDFIRE_CD;
		state[36] = wildfire_cd.ready;
		state[37] = wildfire_cd.time / (float)WILDFIRE_CD;
		state[38] = barrel_cd.ready;
		state[39] = barrel_cd.time / (float)BARREL_CD;
		state[40] = gauss_round_cd.ready;
		state[41] = gauss_round_cd.time / (float)GAUSS_ROUND_CD;
		state[42] = hypercharge_cd.ready;
		state[43] = hypercharge_cd.time / (float)HYPERCHARGE_CD;
		state[44] = ricochet_cd.ready;
		state[45] = ricochet_cd.time / (float)RICOCHET_CD;
		state[46] = stabilizer_cd.ready;
		state[47] = stabilizer_cd.time / (float)STABILIZER_CD;
		state[48] = flamethrower_cd.ready;
		state[49] = flamethrower_cd.time / (float)FLAMETHROWER_CD;
		state[50] = overdrive_cd.ready;
		state[51] = overdrive_cd.time / (float)OVERDRIVE_CD;
		state[52] = turret_cd.ready;
		state[53] = turret_cd.time / (float)TURRET_CD;
		state[54] = gcd_timer.ready;
		state[55] = gcd_timer.time / 250.0f;
	}
}