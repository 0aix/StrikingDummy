#include "Monk.h"
#include <assert.h>

namespace StrikingDummy
{
	Monk::Monk(Stats& stats) :
		Job(stats, MONK_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD)))),
		gl1_base_gcd(lround(floor(0.1f * floor(0.95f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl2_base_gcd(lround(floor(0.1f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl3_base_gcd(lround(floor(0.1f * floor(0.85f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl4_base_gcd(lround(floor(0.1f * floor(0.80f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		fire_gcd(lround(floor(0.1f * floor(1.15f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl1_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.95f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl2_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl3_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.85f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl4_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.80f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay)))),
		gl1_auto_gcd(lround(floor(0.1f * floor(0.95f * floor(1000.0f * this->stats.auto_delay))))),
		gl2_auto_gcd(lround(floor(0.1f * floor(0.90f * floor(1000.0f * this->stats.auto_delay))))),
		gl3_auto_gcd(lround(floor(0.1f * floor(0.85f * floor(1000.0f * this->stats.auto_delay))))),
		gl4_auto_gcd(lround(floor(0.1f * floor(0.80f * floor(1000.0f * this->stats.auto_delay)))))
	{
		crit_expected_multiplier = this->stats.crit_multiplier * ((1.0f - this->stats.dhit_rate) + 1.25f * this->stats.dhit_rate);
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Monk::reset()
	{
		timeline = {};

		fists = Fists::FIRE;
		chakra = 5;
		tackle_procs = 1;

		dot_timer.reset(tick(rng), false);
		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		anatman_timer.reset(tick(rng), false);
		tackle_timer.reset(TACKLE_TIMER, false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);
		timeline.push_event(anatman_timer.time);
		timeline.push_event(tackle_timer.time);

		// buffs
		form.reset(FORM_DURATION, Form::COEURL);
		timeline.push_event(form.time);

		gl.reset(0, 0);
		lf.reset(0, 0);
		twin.reset(0, 0);
		rof.reset(0, 0);
		bro.reset(0, 0);
		dot.reset(0, 0);

		anatman = false;
		skip_anatman_tick = false;

		// cooldowns
		fists_cd.reset(0, true);
		pb_cd.reset(0, true);
		elixir_cd.reset(0, true);
		tk_cd.reset(0, true);
		rof_cd.reset(0, true);
		bro_cd.reset(0, true);
		anatman_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		action_timer.reset(0, true);

		// metrics
		total_damage = 0.0f;
		tk_count = 0;
		sss_count = 0;
		anatman_count = 0;

		history.clear();

		update_history();
	}

	void Monk::update(int elapsed)
	{
		assert(elapsed > 0);

		// server ticks
		dot_timer.update(elapsed);
		if (!anatman)
			auto_timer.update(elapsed);
		anatman_timer.update(elapsed);
		tackle_timer.update(elapsed);

		// buffs
		form.update(elapsed);
		if (!anatman)
			gl.update(elapsed);
		lf.update(elapsed);
		twin.update(elapsed);
		rof.update(elapsed);
		bro.update(elapsed);
		dot.update(elapsed);

		// cooldowns
		fists_cd.update(elapsed);
		pb_cd.update(elapsed);
		elixir_cd.update(elapsed);
		tk_cd.update(elapsed);
		rof_cd.update(elapsed);
		bro_cd.update(elapsed);
		anatman_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		action_timer.update(elapsed);

		// 
		if (dot_timer.ready)
			update_dot();
		if (auto_timer.ready)
			update_auto();
		if (anatman_timer.ready)
			update_anatman();
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
		switch (gl.count)
		{
		case 0:
			auto_timer.reset(auto_gcd, false);
			break;
		case 1:
			auto_timer.reset(gl1_auto_gcd, false);
			break;
		case 2:
			auto_timer.reset(gl2_auto_gcd, false);
			break;
		case 3:
			auto_timer.reset(gl3_auto_gcd, false);
			break;
		case 4:
			auto_timer.reset(gl4_auto_gcd, false);
		}
		push_event(auto_timer.time);
	}

	void Monk::update_anatman()
	{
		if (anatman)
		{
			if (!skip_anatman_tick)
			{
				gl.reset(GL_DURATION, std::min(gl.count + 1, fists == WIND ? 4 : 3));
				push_event(GL_DURATION);
			}
			skip_anatman_tick = false;
		}
		anatman_timer.reset(TICK_TIMER, false);
		push_event(TICK_TIMER);
	}

	int Monk::get_gcd_time() const
	{
		switch (gl.count)
		{
		case 0:
			return rof.count > 0 ? fire_gcd : base_gcd;
		case 1:
			return rof.count > 0 ? gl1_fire_gcd : gl1_base_gcd;
		case 2:
			return rof.count > 0 ? gl2_fire_gcd : gl2_base_gcd;
		case 3:
			return rof.count > 0 ? gl3_fire_gcd : gl3_base_gcd;
		case 4:
			return rof.count > 0 ? gl4_fire_gcd : gl4_base_gcd;
		}
	}

	bool Monk::can_use_action(int action) const
	{
		switch (action)
		{
		case NONE:
			return (gcd_timer.time % (ANIMATION_LOCK + ACTION_TAX)) > 0;
			//return !gcd_timer.ready;
		case BOOTSHINE:
			return gcd_timer.ready && (form.count != Form::RAPTOR && form.count != Form::COEURL);
		case TRUESTRIKE:
			return gcd_timer.ready && (form.count == Form::RAPTOR || form.count == Form::PERFECT);
		case SNAPPUNCH:
			return gcd_timer.ready && (form.count == Form::COEURL || form.count == Form::PERFECT);
		case DRAGONKICK:
			return gcd_timer.ready && (form.count != Form::RAPTOR && form.count != Form::COEURL);
		case TWINSNAKES:
			return gcd_timer.ready && (form.count == Form::RAPTOR || form.count == Form::PERFECT);
		case DEMOLISH:
			return gcd_timer.ready && (form.count == Form::COEURL || form.count == Form::PERFECT);
		case SIXSIDEDSTAR:
			return gcd_timer.ready;
		case SHOULDERTACKLE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tackle_procs > 0;
		case FORBIDDENCHAKRA:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && chakra == 5;
		case ELIXIRFIELD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && elixir_cd.ready;
		case TORNADOKICK:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tk_cd.ready && gl.count == (fists == WIND ? 4 : 3);
		case FISTSOFWIND:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && fists_cd.ready && fists != Fists::WIND;
		case FISTSOFFIRE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && fists_cd.ready && fists != Fists::FIRE;
		case PERFECTBALANCE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && pb_cd.ready;
		case BROTHERHOOD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && bro_cd.ready;
		case RIDDLEOFFIRE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && rof_cd.ready;
		case ANATMAN:
			return ((gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && anatman_cd.ready) || anatman) && gl.count < (fists == WIND ? 4 : 3);
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
			if (action == TORNADOKICK)
				tk_count++;
			else if (action == SIXSIDEDSTAR)
				sss_count++;
			use_damage_action(action);
			break;
		case FISTSOFWIND:
			fists = Fists::WIND;
			fists_cd.reset(FISTS_CD, false);
			push_event(FISTS_CD);
			break;
		case FISTSOFFIRE:
			fists = Fists::FIRE;
			gl.count = std::min(gl.count, 3);
			fists_cd.reset(FISTS_CD, false);
			push_event(FISTS_CD);
			break;
		case PERFECTBALANCE:
			form.reset(FORM_DURATION, Form::PERFECT);
			pb_cd.reset(PB_CD, false);
			push_event(FORM_DURATION);
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
		case ANATMAN:
			anatman_count++;
			anatman = true;
			anatman_cd.reset(ANATMAN_CD, false);
			push_event(ANATMAN_CD);
			skip_anatman_tick = anatman_timer.time <= ANIMATION_LOCK + ACTION_TAX;
			action_timer.reset(skip_anatman_tick ? anatman_timer.time + TICK_TIMER : anatman_timer.time, false);
			push_event(action_timer.time);
			return;
		case WAIT:
			break;
		}
		if (anatman)
		{
			assert(auto_timer.time > 0);
			push_event(auto_timer.time);
			if (gl.count > 0)
			{
				assert(gl.time > 0);
				push_event(gl.time);
			}
			anatman = false;
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
			if (form.count == Form::OPOOPO || form.count == Form::PERFECT)
				is_crit = true;
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::RAPTOR);
				push_event(FORM_DURATION);
			}
			lf.reset(0, 0);
			weaponskill = true;
			break;
		case TRUESTRIKE:
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::COEURL);
				push_event(FORM_DURATION);
			}
			weaponskill = true;
			break;
		case SNAPPUNCH:
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::OPOOPO);
				push_event(FORM_DURATION);
			}
			gl.reset(GL_DURATION, std::min(gl.count + 1, fists == WIND ? 4 : 3));
			push_event(GL_DURATION);
			weaponskill = true;
			break;
		case DRAGONKICK:
			if (form.count == Form::OPOOPO || form.count == Form::PERFECT)
			{
				lf.reset(LF_DURATION, 1);
				push_event(LF_DURATION);
			}
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::RAPTOR);
				push_event(FORM_DURATION);
			}
			weaponskill = true;
			break;
		case TWINSNAKES:
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::COEURL);
				push_event(FORM_DURATION);
			}
			twin.reset(TWIN_DURATION, 1);
			push_event(TWIN_DURATION);
			weaponskill = true;
			break;
		case DEMOLISH:
			if (form.count != Form::PERFECT)
			{
				form.reset(FORM_DURATION, Form::OPOOPO);
				push_event(FORM_DURATION);
			}
			dot_gl = gl.count;
			dot_fof = fists == FISTSOFFIRE;
			dot_twin = twin.count > 0;
			dot_bro = bro.count > 0;
			dot_rof = rof.count > 0;
			dot.reset(DOT_DURATION, 1);
			gl.reset(GL_DURATION, std::min(gl.count + 1, fists == WIND ? 4 : 3));
			push_event(DOT_DURATION);
			push_event(GL_DURATION);
			weaponskill = true;
			break;
		case SIXSIDEDSTAR:
			if (gl.count > 0)
			{
				gl.time = GL_DURATION;
				push_event(GL_DURATION);
			}
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
			gl.reset(0, 0);
			tk_cd.reset(TK_CD, false);
			push_event(TK_CD);
			break;
		}
		if (weaponskill)
		{
			if ((is_crit || prob(rng) < stats.crit_rate) && prob(rng) < MEDITATION_PROC_RATE && chakra < 5)
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
			if (form.count == Form::OPOOPO || form.count == Form::PERFECT)
				is_crit = true;
			potency = lf.count > 0 ? LF_BOOTSHINE_POTENCY : BOOTSHINE_POTENCY;
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
		float damage = potency * stats.potency_multiplier * GL_MULTIPLIER[gl.count] *
			(fists == Fists::FIRE ? FOF_MULTIPLIER : 1.0f) *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f);
		if (is_crit)
			return damage * crit_expected_multiplier;
		return damage * stats.expected_multiplier;
	}

	float Monk::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return DEMOLISH_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * GL_MULTIPLIER[dot_gl] *
			(dot_fof ? FOF_MULTIPLIER : 1.0f) *
			(dot_twin ? TWIN_MULTIPLIER : 1.0f) *
			(dot_bro ? BRO_MULTIPLIER : 1.0f) *
			(dot_rof ? ROF_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	float Monk::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier * GL_MULTIPLIER[gl.count] *
			(fists == Fists::FIRE ? FOF_MULTIPLIER : 1.0f) *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	void Monk::get_state(float* state)
	{
		state[0] = fists == Fists::WIND;
		state[1] = fists == Fists::FIRE;
		state[2] = 0.2f * chakra;
		state[3] = form.count == Form::NORMAL;
		state[4] = form.count == Form::OPOOPO;
		state[5] = form.count == Form::RAPTOR;
		state[6] = form.count == Form::COEURL;
		state[7] = form.count == Form::PERFECT;
		state[8] = form.time / (float)FORM_DURATION;
		state[9] = gl.count == 1;
		state[10] = gl.count == 2;
		state[11] = gl.count == 3;
		state[12] = gl.count == 4;
		state[13] = gl.time / (float)GL_DURATION;
		state[14] = lf.count > 0;
		state[15] = lf.time / (float)LF_DURATION;
		state[16] = twin.count > 0;
		state[17] = twin.time / (float)TWIN_DURATION;
		state[18] = rof.count > 0;
		state[19] = rof.time / (float)ROF_DURATION;
		state[20] = bro.count > 0;
		state[21] = bro.time / (float)BRO_DURATION;
		state[22] = dot.count > 0;
		state[23] = dot.time / (float)DOT_DURATION;
		state[24] = dot.count > 0 && dot_gl == 1;
		state[25] = dot.count > 0 && dot_gl == 2;
		state[26] = dot.count > 0 && dot_gl == 3;
		state[27] = dot.count > 0 && dot_gl == 4;
		state[28] = dot.count > 0 && dot_fof;
		state[29] = dot.count > 0 && dot_twin;
		state[30] = dot.count > 0 && dot_bro;
		state[31] = dot.count > 0 && dot_rof;
		state[32] = anatman;
		state[33] = tackle_procs > 0;
		state[34] = tackle_procs > 1;
		state[35] = tackle_timer.time / (float)TACKLE_TIMER;
		state[36] = fists_cd.ready;
		state[37] = fists_cd.time / (float)FISTS_CD;
		state[38] = pb_cd.ready;
		state[39] = pb_cd.time / (float)PB_CD;
		state[40] = elixir_cd.ready;
		state[41] = elixir_cd.time / (float)ELIXIR_CD;
		state[42] = tk_cd.ready;
		state[43] = tk_cd.time / (float)TK_CD;
		state[44] = rof_cd.ready;
		state[45] = rof_cd.time / (float)ROF_CD;
		state[46] = bro_cd.ready;
		state[47] = bro_cd.time / (float)BRO_CD;
		state[48] = anatman_cd.ready;
		state[49] = anatman_cd.time / (float)ANATMAN_CD;
		state[50] = gcd_timer.ready;
		state[51] = gcd_timer.time / 500.0f;
	}

	std::string Monk::get_info()
	{
		return std::string();
	}
}