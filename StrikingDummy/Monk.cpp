#include "Monk.h"
#include <assert.h>

namespace StrikingDummy
{
	Monk::Monk(Stats& stats) :
		Job(stats, MONK_ATTR),
		base_gcd(lround(floor(0.1f * floor(0.80f * floor(this->stats.ss_multiplier * BASE_GCD)))) * 10),
		auto_gcd(lround(floor(0.1f * floor(0.80f * floor(1000.0f * this->stats.auto_delay)))) * 10)
	{
		crit_expected_multiplier = this->stats.crit_multiplier * ((1.0f - this->stats.dhit_rate) + 1.25f * this->stats.dhit_rate);
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Monk::reset()
	{
		timeline = {};

		chakra = 5;
		tackle_procs = 2;

		dot_timer.reset(tick(rng), false);
		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		medi_timer.reset(std::uniform_int_distribution<int>(1, MEDI_TIMER)(rng), false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);
		timeline.push_event(medi_timer.time);

		tackle_timer.reset(0, false);

		// buffs
		form = Form::OPOOPO;

		twin.reset(0, 0);
		rof.reset(0, 0);
		bro.reset(0, 0);
		dot.reset(0, 0);

		// cooldowns
		pb_cd.reset(0, true);
		elixir_cd.reset(0, true);
		tk_cd.reset(0, true);
		rof_cd.reset(0, true);
		bro_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		action_timer.reset(0, true);

		// precast pot
		pot.reset(POT_DURATION - POTION_LOCK, 1);
		pot_cd.reset(POT_CD - POTION_LOCK, false);
		timeline.push_event(pot.time);
		timeline.push_event(pot_cd.time);

		// metrics
		total_damage = 0.0f;
		sss_count = 0;

		history.clear();

		update_history();
	}

	void Monk::update(int elapsed)
	{
		assert(elapsed > 0);

		// server ticks
		dot_timer.update(elapsed);
		auto_timer.update(elapsed);
		tackle_timer.update(elapsed);

		// buffs
		twin.update(elapsed);
		rof.update(elapsed);
		bro.update(elapsed);
		dot.update(elapsed);
		pot.update(elapsed);

		// cooldowns
		pb_cd.update(elapsed);
		elixir_cd.update(elapsed);
		tk_cd.update(elapsed);
		rof_cd.update(elapsed);
		bro_cd.update(elapsed);
		pot_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		action_timer.update(elapsed);
		medi_timer.update(elapsed);

		// 
		if (dot_timer.ready)
			update_dot();
		if (auto_timer.ready)
			update_auto();
		if (medi_timer.ready)
			update_medi();
		if (tackle_timer.ready)
		{
			tackle_procs++;
			assert(tackle_timer.time == 0);
			assert(tackle_procs <= 2);
			tackle_timer.ready = false;
			if (tackle_procs < 2)
			{
				tackle_timer.time = TACKLE_TIMER;
				push_event(TACKLE_TIMER);
			}
		}

		update_history();
	}

	void Monk::update_history()
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

	void Monk::update_dot()
	{
		if (dot.count > 0)
		{
			float damage = get_dot_damage();
			total_damage += damage;
			history.back().reward += damage;
		}
		dot_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	void Monk::update_auto()
	{
		float damage = get_auto_damage();
		total_damage += damage;
		history.back().reward += damage;
		auto_timer.reset(auto_gcd, false);
		push_event(auto_timer.time);
	}

	void Monk::update_medi()
	{
		if (bro.count > 0 && prob(rng) < BRO_PROC_RATE)
			chakra = std::min(chakra + 1, 5);
		medi_timer.reset(MEDI_TIMER, false);
		push_event(MEDI_TIMER);
	}

	int Monk::get_gcd_time() const
	{
		return base_gcd;
	}

	bool Monk::can_use_action(int action) const
	{
		switch (action)
		{
		case NONE:
			return (gcd_timer.time % (ANIMATION_LOCK + ACTION_TAX)) > 0;
			//return !gcd_timer.ready;
		case BOOTSHINE:
			return gcd_timer.ready && (form != Form::RAPTOR && form != Form::COEURL);
		case TRUESTRIKE:
			return gcd_timer.ready && (form == Form::RAPTOR || form == Form::PERFECT);
		case SNAPPUNCH:
			return gcd_timer.ready && (form == Form::COEURL || form == Form::PERFECT);
		case DRAGONKICK:
			return gcd_timer.ready && (form != Form::RAPTOR && form != Form::COEURL);
		case TWINSNAKES:
			return gcd_timer.ready && (form == Form::RAPTOR || form == Form::PERFECT);
		case DEMOLISH:
			return gcd_timer.ready && (form == Form::COEURL || form == Form::PERFECT);
		case SIXSIDEDSTAR:
			return gcd_timer.ready;
		case SHOULDERTACKLE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tackle_procs > 0;
		case FORBIDDENCHAKRA:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && chakra == 5;
		case ELIXIRFIELD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && elixir_cd.ready;
		case TORNADOKICK:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tk_cd.ready;
		case PERFECTBALANCE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && pb_cd.ready;
		case BROTHERHOOD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && bro_cd.ready;
		case RIDDLEOFFIRE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && rof_cd.ready;
		case POT:
			return gcd_timer.time >= POTION_LOCK + ACTION_TAX && pot_cd.ready;
		case WAIT:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
			//return !gcd_timer.ready && gcd_timer.time >= 2 * (ANIMATION_LOCK + ACTION_TAX);
		}
		return false;
	}

	void Monk::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
			action_timer.reset(gcd_timer.time % (ANIMATION_LOCK + ACTION_TAX), false);
			push_event(action_timer.time);
			return;
		case BOOTSHINE:
		case TRUESTRIKE:
		case SNAPPUNCH:
		case DRAGONKICK:
		case TWINSNAKES:
		case DEMOLISH:
		case SIXSIDEDSTAR:
		case SHOULDERTACKLE:
		case FORBIDDENCHAKRA:
		case ELIXIRFIELD:
		case TORNADOKICK:
			if (action == SIXSIDEDSTAR)
				sss_count++;
			use_damage_action(action);
			break;
		case PERFECTBALANCE:
			form = Form::PERFECT;
			pb_count = 6;
			pb_cd.reset(PB_CD, false);
			push_event(PB_CD);
			break;
		case BROTHERHOOD:
			bro.reset(BRO_DURATION, 1);
			bro_cd.reset(BRO_CD, false);
			push_event(BRO_DURATION);
			push_event(BRO_CD);
			break;
		case RIDDLEOFFIRE:
			rof.reset(ROF_DURATION, 1);
			rof_cd.reset(ROF_CD, false);
			push_event(ROF_DURATION);
			push_event(ROF_CD);
			break;
		case POT:
			pot.reset(POT_DURATION, 1);
			pot_cd.reset(POT_CD, false);
			push_event(POT_DURATION);
			push_event(POT_CD);
			action_timer.reset(POTION_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			return;
		case WAIT:
			break;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
	}

	void Monk::use_damage_action(int action)
	{
		float damage = get_damage(action);
		total_damage += damage;
		history.back().reward += damage;

		bool weaponskill = false;
		bool is_crit = false; // bootshine only
		int gcd_time = get_gcd_time();

		switch (action)
		{
		case BOOTSHINE:
			if (form == Form::OPOOPO || form == Form::PERFECT)
				is_crit = true;
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::RAPTOR;
			lf = false;
			weaponskill = true;
			break;
		case TRUESTRIKE:
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::COEURL;
			weaponskill = true;
			break;
		case SNAPPUNCH:
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::OPOOPO;
			weaponskill = true;
			break;
		case DRAGONKICK:
			if (form == Form::OPOOPO || form == Form::PERFECT)
				lf = true;
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::RAPTOR;
			weaponskill = true;
			break;
		case TWINSNAKES:
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::COEURL;
			twin.reset(TWIN_DURATION, 1);
			push_event(TWIN_DURATION);
			weaponskill = true;
			break;
		case DEMOLISH:
			if (form == Form::PERFECT)
			{
				if (--pb_count == 0)
					form = Form::NORMAL;
			}
			else
				form = Form::OPOOPO;
			dot_twin = twin.count > 0;
			dot_bro = bro.count > 0;
			dot_rof = rof.count > 0;
			dot_pot = pot.count > 0;
			dot.reset(DOT_DURATION, 1);
			push_event(DOT_DURATION);
			weaponskill = true;
			break;
		case SIXSIDEDSTAR:
			gcd_time *= 2;
			weaponskill = true;
			break;
		case SHOULDERTACKLE:
			tackle_procs--;
			if (tackle_timer.time == 0)
			{
				assert(!tackle_timer.ready);
				tackle_timer.reset(TACKLE_TIMER, false);
				push_event(TACKLE_TIMER);
			}
			break;
		case FORBIDDENCHAKRA:
			chakra = 0;
			break;
		case ELIXIRFIELD:
			elixir_cd.reset(ELIXIR_CD, false);
			push_event(ELIXIR_CD);
			break;
		case TORNADOKICK:
			tk_cd.reset(TK_CD, false);
			push_event(TK_CD);
			break;
		}
		if (weaponskill)
		{
			if ((is_crit || prob(rng) < stats.crit_rate) && chakra < 5)
				chakra++;
			if (bro.count > 0 && prob(rng) < BRO_PROC_RATE && chakra < 5)
				chakra++;
			gcd_timer.reset(gcd_time, false);
			push_event(gcd_time);
		}
	}

	float Monk::get_damage(int action) const
	{
		float potency = 0.0f;
		bool is_crit = false;
		switch (action)
		{
		case BOOTSHINE:
			if (form == Form::OPOOPO || form == Form::PERFECT)
				is_crit = true;
			potency = lf ? LF_BOOTSHINE_POTENCY : BOOTSHINE_POTENCY;
			break;
		case TRUESTRIKE:
			potency = TRUESTRIKE_POTENCY;
			break;
		case SNAPPUNCH:
			potency = SNAPPUNCH_POTENCY;
			break;
		case DRAGONKICK:
			potency = DRAGONKICK_POTENCY;
			break;
		case TWINSNAKES:
			potency = TWINSNAKES_POTENCY;
			break;
		case DEMOLISH:
			potency = DEMOLISH_POTENCY;
			break;
		case SIXSIDEDSTAR:
			potency = SIXSIDEDSTAR_POTENCY;
			break;
		case SHOULDERTACKLE:
			potency = SHOULDERTACKLE_POTENCY;
			break;
		case FORBIDDENCHAKRA:
			potency = FORBIDDENCHAKRA_POTENCY;
			break;
		case ELIXIRFIELD:
			potency = ELIXIRFIELD_POTENCY;
			break;
		case TORNADOKICK:
			potency = TORNADOKICK_POTENCY;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		float damage = potency * stats.potency_multiplier * FOF_MULTIPLIER *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f) *
			(pot.count > 0 ? stats.pot_multiplier : 1.0f);
		if (is_crit)
			return damage * crit_expected_multiplier;
		return damage * stats.expected_multiplier;
	}

	float Monk::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return DEMOLISH_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * FOF_MULTIPLIER *
			(dot_twin ? TWIN_MULTIPLIER : 1.0f) *
			(dot_bro ? BRO_MULTIPLIER : 1.0f) *
			(dot_rof ? ROF_MULTIPLIER : 1.0f) *
			(dot_pot ? stats.pot_multiplier : 1.0f) *
			stats.expected_multiplier;
	}

	float Monk::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier * FOF_MULTIPLIER *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f) *
			(pot.count > 0 ? stats.pot_multiplier : 1.0f) *
			stats.expected_multiplier;
	}

	void Monk::get_state(float* state)
	{
		state[0] = 0.2f * chakra;
		state[1] = form == Form::NORMAL;
		state[2] = form == Form::OPOOPO;
		state[3] = form == Form::RAPTOR;
		state[4] = form == Form::COEURL;
		state[5] = form == Form::PERFECT;
		state[6] = pb_count / 6.0f;
		state[7] = lf;
		state[8] = twin.count > 0;
		state[9] = twin.time / (float)TWIN_DURATION;
		state[10] = rof.count > 0;
		state[11] = rof.time / (float)ROF_DURATION;
		state[12] = bro.count > 0;
		state[13] = bro.time / (float)BRO_DURATION;
		state[14] = dot.count > 0;
		state[15] = dot.time / (float)DOT_DURATION;
		state[16] = dot.count > 0 && dot_twin;
		state[17] = dot.count > 0 && dot_bro;
		state[18] = dot.count > 0 && dot_rof;
		state[19] = dot.count > 0 && dot_pot;
		state[20] = tackle_procs > 0;
		state[21] = tackle_procs > 1;
		state[22] = tackle_timer.time / (float)TACKLE_TIMER;
		state[23] = pb_cd.ready;
		state[24] = pb_cd.time / (float)PB_CD;
		state[25] = elixir_cd.ready;
		state[26] = elixir_cd.time / (float)ELIXIR_CD;
		state[27] = tk_cd.ready;
		state[28] = tk_cd.time / (float)TK_CD;
		state[29] = rof_cd.ready;
		state[30] = rof_cd.time / (float)ROF_CD;
		state[31] = bro_cd.ready;
		state[32] = bro_cd.time / (float)BRO_CD;
		state[33] = pot.count > 0;
		state[34] = pot.time / (float)POT_DURATION;
		state[35] = pot_cd.ready;
		state[36] = pot_cd.time / (float)POT_CD;
		state[37] = gcd_timer.ready;
		state[38] = gcd_timer.time / (BASE_GCD * 2000.0f);
	}

	std::string Monk::get_info()
	{
		return std::string();
	}
}