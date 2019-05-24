#include "Mimu.h"
#include <assert.h>

namespace StrikingDummy
{
	Mimu::Mimu(Stats& stats) :
		Job(stats, MIMU_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD)))),
		gl1_base_gcd(lround(floor(0.1f * floor(0.95f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl2_base_gcd(lround(floor(0.1f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl3_base_gcd(lround(floor(0.1f * floor(0.85f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		fire_gcd(lround(floor(0.1f * floor(1.15f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl1_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.95f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl2_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl3_fire_gcd(lround(floor(0.1f * floor(1.15f * floor(0.85f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay)))),
		gl1_auto_gcd(lround(floor(0.1f * floor(0.95f * floor(1000.0f * this->stats.auto_delay))))),
		gl2_auto_gcd(lround(floor(0.1f * floor(0.90f * floor(1000.0f * this->stats.auto_delay))))),
		gl3_auto_gcd(lround(floor(0.1f * floor(0.85f * floor(1000.0f * this->stats.auto_delay)))))
	{
		float ir_crit_rate = std::min(this->stats.crit_rate + IR_CRIT_RATE, 1.0f);
		float dcrit_rate = ir_crit_rate * this->stats.dhit_rate;

		ir_expected_multiplier = (1.0f - ir_crit_rate + dcrit_rate - this->stats.dhit_rate) + this->stats.crit_multiplier * (ir_crit_rate - dcrit_rate) + this->stats.crit_multiplier * 1.25f * dcrit_rate + 1.25f * (this->stats.dhit_rate - dcrit_rate);
		crit_expected_multiplier = this->stats.crit_multiplier * ((1.0f - this->stats.dhit_rate) + 1.25f * this->stats.dhit_rate);

		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Mimu::reset()
	{
		timeline = {};

		fists = Fists::WIND;
		chakra = 5;

		dot_timer.reset(tick(rng), false);
		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);

		// buffs
		form.reset(FORM_DURATION, Form::COEURL);
		timeline.push_event(form.time);

		gl.reset(0, 0);
		twin.reset(0, 0);
		ir.reset(0, 0);
		dk.reset(0, 0);
		row.reset(0, 0);
		rof.reset(0, 0);
		bro.reset(0, 0);
		dot.reset(0, 0);

		// cooldowns
		ir_cd.reset(0, true);
		fists_cd.reset(0, true);
		tackle_cd.reset(0, true);
		steel_cd.reset(0, true);
		howling_cd.reset(0, true);
		pb_cd.reset(0, true);
		chakra_cd.reset(0, true);
		elixir_cd.reset(0, true);
		tk_cd.reset(0, true);
		rof_cd.reset(0, true);
		bro_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		action_timer.reset(0, true);

		// metrics
		total_damage = 0.0f;
		tk_count = 0;

		history.clear();

		update_history();
	}

	void Mimu::update(int elapsed)
	{
		assert(elapsed > 0);

		// server ticks
		dot_timer.update(elapsed);
		auto_timer.update(elapsed);

		// buffs
		form.update(elapsed);
		gl.update(elapsed);
		twin.update(elapsed);
		ir.update(elapsed);
		dk.update(elapsed);
		row.update(elapsed);
		rof.update(elapsed);
		bro.update(elapsed);
		dot.update(elapsed);

		// cooldowns
		ir_cd.update(elapsed);
		fists_cd.update(elapsed);
		tackle_cd.update(elapsed);
		steel_cd.update(elapsed);
		howling_cd.update(elapsed);
		pb_cd.update(elapsed);
		chakra_cd.update(elapsed);
		elixir_cd.update(elapsed);
		tk_cd.update(elapsed);
		rof_cd.update(elapsed);
		bro_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		action_timer.update(elapsed);

		// 
		if (dot_timer.ready)
			update_dot();
		if (auto_timer.ready)
			update_auto();

		update_history();
	}

	void Mimu::update_history()
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

	void Mimu::update_dot()
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

	void Mimu::update_auto()
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
		}
		push_event(auto_timer.time);
	}

	int Mimu::get_gcd_time() const
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
		}
	}

	bool Mimu::can_use_action(int action) const
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
		case FISTSOFWIND:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && fists_cd.ready && fists != Fists::WIND;
		case FISTSOFFIRE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && fists_cd.ready && fists != Fists::FIRE;
		case INTERNALRELEASE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && ir_cd.ready;
		case PERFECTBALANCE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && pb_cd.ready;
		case BROTHERHOOD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && bro_cd.ready;
		case STEELPEAK:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && steel_cd.ready;
		case HOWLINGFIST:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && howling_cd.ready;
		case FORBIDDENCHAKRA:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && chakra_cd.ready && chakra == 5;
		case ELIXIRFIELD:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && elixir_cd.ready;
		case TORNADOKICK:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tk_cd.ready && gl.count == 3;
		case RIDDLEOFWIND:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && row.count > 0;
		case RIDDLEOFFIRE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && rof_cd.ready && fists == Fists::FIRE;
		case WINDTACKLE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tackle_cd.ready && fists == Fists::WIND;
		case FIRETACKLE:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX && tackle_cd.ready && fists == Fists::FIRE;
		case WAIT:
			return gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
			//return !gcd_timer.ready && gcd_timer.time >= 2 * (ANIMATION_LOCK + ACTION_TAX);
		}
		return false;
	}

	void Mimu::use_action(int action)
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
		case STEELPEAK:
		case HOWLINGFIST:
		case FORBIDDENCHAKRA:
		case ELIXIRFIELD:
		case TORNADOKICK:
		case RIDDLEOFWIND:
		case WINDTACKLE:
		case FIRETACKLE:
			if (action == TORNADOKICK)
				tk_count++;
			use_damage_action(action);
			break;
		case FISTSOFWIND:
			fists = Fists::WIND;
			rof.reset(0, 0);
			fists_cd.reset(FISTS_CD, false);
			push_event(FISTS_CD);
			break;
		case FISTSOFFIRE:
			fists = Fists::FIRE;
			row.reset(0, 0);
			fists_cd.reset(FISTS_CD, false);
			push_event(FISTS_CD);
			break;
		case INTERNALRELEASE:
			ir.reset(IR_DURATION, 1);
			ir_cd.reset(IR_CD, false);
			push_event(IR_DURATION);
			push_event(IR_CD);
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
			fists = Fists::FIRE;
			row.reset(0, 0);
			rof.reset(ROF_DURATION, 1);
			rof_cd.reset(ROF_CD, false);
			push_event(ROF_DURATION);
			push_event(ROF_CD);
			break;
		case WAIT:
			break;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
	}

	void Mimu::use_damage_action(int action)
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
			gl.reset(GL_DURATION, std::min(gl.count + 1, 3));
			push_event(GL_DURATION);
			weaponskill = true;
			break;
		case DRAGONKICK:
			if (form.count == Form::OPOOPO || form.count == Form::PERFECT)
			{
				dk.reset(DK_DURATION, 1);
				push_event(DK_DURATION);
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
			dot_dk = dk.count > 0;
			dot_bro = bro.count > 0;
			dot_rof = rof.count > 0;
			dot_ir = ir.count > 0;
			dot.reset(DOT_DURATION, 1);
			gl.reset(GL_DURATION, std::min(gl.count + 1, 3));
			push_event(DOT_DURATION);
			push_event(GL_DURATION);
			weaponskill = true;
			break;
		case STEELPEAK:
			steel_cd.reset(STEEL_CD, false);
			push_event(STEEL_CD);
			break;
		case HOWLINGFIST:
			howling_cd.reset(HOWLING_CD, false);
			push_event(HOWLING_CD);
			break;
		case FORBIDDENCHAKRA:
			chakra = 0;
			chakra_cd.reset(CHAKRA_CD, false);
			push_event(CHAKRA_CD);
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
		case RIDDLEOFWIND:
			row.reset(0, 0);
			gl.reset(GL_DURATION, std::min(gl.count + 1, 3));
			push_event(GL_DURATION);
			break;
		case WINDTACKLE:
			tackle_cd.reset(TACKLE_CD, false);
			row.reset(ROW_DURATION, 1);
			push_event(TACKLE_CD);
			push_event(ROW_DURATION);
			break;
		case FIRETACKLE:
			tackle_cd.reset(TACKLE_CD, false);
			push_event(TACKLE_CD);
			break;
		}
		if (weaponskill)
		{
			if ((is_crit || prob(rng) < stats.crit_rate + (ir.count > 0 ? IR_CRIT_RATE : 0.0f)) && prob(rng) < MEDITATION_PROC_RATE && chakra < 5)
				chakra++;
			if (bro.count > 0 && prob(rng) < BRO_PROC_RATE && chakra < 5)
				chakra++;
			gcd_timer.reset(gcd_time, false);
			push_event(gcd_time);
		}
	}

	float Mimu::get_damage(int action) const
	{
		float potency = 0.0f;
		bool is_crit = false;
		switch (action)
		{
		case BOOTSHINE:
			if (form.count == Form::OPOOPO || form.count == Form::PERFECT)
				is_crit = true;
			potency = BOOT_POTENCY;
			break;
		case TRUESTRIKE:
			potency = TRUE_POTENCY;
			break;
		case SNAPPUNCH:
			potency = SNAP_POTENCY;
			break;
		case DRAGONKICK:
			potency = DRAGON_POTENCY;
			break;
		case TWINSNAKES:
			potency = TWIN_POTENCY;
			break;
		case DEMOLISH:
			potency = DEMO_POTENCY;
			break;
		case STEELPEAK:
			potency = STEEL_POTENCY;
			break;
		case HOWLINGFIST:
			potency = HOWLING_POTENCY;
			break;
		case FORBIDDENCHAKRA:
			potency = CHAKRA_POTENCY;
			break;
		case ELIXIRFIELD:
			potency = ELIXIR_POTENCY;
			break;
		case TORNADOKICK:
			potency = TK_POTENCY;
			break;
		case RIDDLEOFWIND:
			potency = WINDTACKLE_POTENCY;
			break;
		case WINDTACKLE:
			potency = WINDTACKLE_POTENCY;
			break;
		case FIRETACKLE:
			potency = FIRETACKLE_POTENCY;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		float damage = potency * stats.potency_multiplier * GL_MULTIPLIER[gl.count] *
			(fists == Fists::FIRE ? FOF_MULTIPLIER : 1.0f) *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(dk.count > 0 ? DK_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f);
		if (is_crit)
			return damage * crit_expected_multiplier;
		else if (ir.count > 0)
			return damage * ir_expected_multiplier;
		return damage * stats.expected_multiplier;
	}

	float Mimu::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return DEMO_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * GL_MULTIPLIER[dot_gl] *
			(dot_fof ? FOF_MULTIPLIER : 1.0f) *
			(dot_twin ? TWIN_MULTIPLIER : 1.0f) *
			(dot_dk ? DK_MULTIPLIER : 1.0f) *
			(dot_bro ? BRO_MULTIPLIER : 1.0f) *
			(dot_rof ? ROF_MULTIPLIER : 1.0f) *
			(dot_ir ? ir_expected_multiplier : stats.expected_multiplier);
	}

	float Mimu::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier * GL_MULTIPLIER[gl.count] *
			(fists == Fists::FIRE ? FOF_MULTIPLIER : 1.0f) *
			(twin.count > 0 ? TWIN_MULTIPLIER : 1.0f) *
			(dk.count > 0 ? DK_MULTIPLIER : 1.0f) *
			(bro.count > 0 ? BRO_MULTIPLIER : 1.0f) *
			(rof.count > 0 ? ROF_MULTIPLIER : 1.0f) *
			(ir.count > 0 ? ir_expected_multiplier : stats.expected_multiplier);
	}

	void Mimu::get_state(float* state)
	{
		state[0] = fists == Fists::WIND;
		state[1] = fists == Fists::FIRE;
		state[2] = 0.2f * chakra;
		//state[3] = dot_timer.time / (float)TICK_TIMER;
		//state[4] = auto_timer.time / (float)auto_gcd;
		state[3] = form.count == Form::NORMAL;
		state[4] = form.count == Form::OPOOPO;
		state[5] = form.count == Form::RAPTOR;
		state[6] = form.count == Form::COEURL;
		state[7] = form.count == Form::PERFECT;
		state[8] = form.time / (float)FORM_DURATION;
		state[9] = gl.count == 1;
		state[10] = gl.count == 2;
		state[11] = gl.count == 3;
		state[12] = gl.time / (float)GL_DURATION;
		state[13] = twin.count > 0;
		state[14] = twin.time / (float)TWIN_DURATION;
		state[15] = ir.count > 0;
		state[16] = ir.time / (float)IR_DURATION;
		state[17] = dk.count > 0;
		state[18] = dk.time / (float)DK_DURATION;
		state[19] = row.count > 0;
		state[20] = row.time / (float)ROW_DURATION;
		state[21] = rof.count > 0;
		state[22] = rof.time / (float)ROF_DURATION;
		state[23] = bro.count > 0;
		state[24] = bro.time / (float)BRO_DURATION;
		state[25] = dot.count > 0;
		state[26] = dot.time / (float)DOT_DURATION;
		state[27] = dot.count > 0 && dot_gl == 1;
		state[28] = dot.count > 0 && dot_gl == 2;
		state[29] = dot.count > 0 && dot_gl == 3;
		state[30] = dot.count > 0 && dot_fof;
		state[31] = dot.count > 0 && dot_twin;
		state[32] = dot.count > 0 && dot_dk;
		state[33] = dot.count > 0 && dot_bro;
		state[34] = dot.count > 0 && dot_rof;
		state[35] = dot.count > 0 && dot_ir;
		state[36] = ir_cd.ready;
		state[37] = ir_cd.time / (float)IR_CD;
		state[38] = fists_cd.ready;
		state[39] = fists_cd.time / (float)FISTS_CD;
		state[40] = tackle_cd.ready;
		state[41] = tackle_cd.time / (float)TACKLE_CD;
		state[42] = steel_cd.ready;
		state[43] = steel_cd.time / (float)STEEL_CD;
		state[44] = howling_cd.ready;
		state[45] = howling_cd.time / (float)HOWLING_CD;
		state[46] = pb_cd.ready;
		state[47] = pb_cd.time / (float)PB_CD;
		state[48] = chakra_cd.ready;
		state[49] = chakra_cd.time / (float)CHAKRA_CD;
		state[50] = elixir_cd.ready;
		state[51] = elixir_cd.time / (float)ELIXIR_CD;
		state[52] = tk_cd.ready;
		state[53] = tk_cd.time / (float)TK_CD;
		state[54] = rof_cd.ready;
		state[55] = rof_cd.time / (float)ROF_CD;
		state[56] = bro_cd.ready;
		state[57] = bro_cd.time / (float)BRO_CD;
		state[58] = gcd_timer.ready;
		state[59] = gcd_timer.time / 250.0f;
		state[60] = (gcd_timer.time % (ANIMATION_LOCK + ACTION_TAX)) > 0;
		state[61] = (gcd_timer.time / (ANIMATION_LOCK + ACTION_TAX)) == 1;
		state[62] = (gcd_timer.time / (ANIMATION_LOCK + ACTION_TAX)) == 2;
		state[63] = (gcd_timer.time / (ANIMATION_LOCK + ACTION_TAX)) == 3;
	}

	std::string Mimu::get_info()
	{
		return std::string();
	}
}