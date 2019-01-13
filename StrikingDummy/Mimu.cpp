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
		fire_gcd(lround(floor(0.1f * floor(0.5f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		gl1_fire_gcd(lround(floor(0.1f * floor(0.5f * floor(0.95f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl2_fire_gcd(lround(floor(0.1f * floor(0.5f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		gl3_fire_gcd(lround(floor(0.1f * floor(0.5f * floor(0.85f * floor(this->stats.ss_multiplier * BASE_GCD)))))),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay)))),
		gl1_auto_gcd(lround(floor(0.1f * floor(0.95f * floor(1000.0f * this->stats.auto_delay))))),
		gl2_auto_gcd(lround(floor(0.1f * floor(0.90f * floor(1000.0f * this->stats.auto_delay))))),
		gl3_auto_gcd(lround(floor(0.1f * floor(0.85f * floor(1000.0f * this->stats.auto_delay)))))
	{
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
		total_damage = 0;
		tk_count = 0;

		history.clear();

		refresh_state();
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

		refresh_state();
	}

	void Mimu::refresh_state()
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
			t.reward = 0.0f;
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
		default:
			throw 0;
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
		enum Action
		{
			NONE,
			BOOTSHINE, TRUESTRIKE, SNAPPUNCH, DRAGONKICK, TWINSNAKES, DEMOLISH, 
			FISTSOFWIND, FISTSOFFIRE,
			INTERNALRELEASE, PERFECTBALANCE, BROTHERHOOD, STEELPEAK, HOWLINGFIST, FORBIDDENCHAKRA, ELIXIRFIELD, TORNADOKICK,
			RIDDLEOFWIND, RIDDLEOFFIRE, SHOULDERTACKLE, WINDTACKLE, FIRETACKLE
		};
		switch (action)
		{
		case NONE:
			return !gcd_timer.ready;
		case BOOTSHINE:
			return gcd_timer.ready;
		case TRUESTRIKE:
			return gcd_timer.ready && (form.count == Form::RAPTOR || form.count == Form::PERFECT);
		case SNAPPUNCH:
			return gcd_timer.ready && (form.count == Form::COEURL || form.count == Form::PERFECT);
		case DRAGONKICK:
			return gcd_timer.ready;
		case TWINSNAKES:
			return gcd_timer.ready && (form.count == Form::RAPTOR || form.count == Form::PERFECT);
		case DEMOLISH:
			return gcd_timer.ready && (form.count == Form::COEURL || form.count == Form::PERFECT);
		case FISTSOFWIND:
			return fists_cd.ready && fists != Fists::WIND;
		case FISTSOFFIRE:
			return fists_cd.ready && fists != Fists::FIRE;
		case INTERNALRELEASE:
			return ir_cd.ready;
		case PERFECTBALANCE:
			return pb_cd.ready;
		case BROTHERHOOD:
			return bro_cd.ready;
		case STEELPEAK:
			return steel_cd.ready;
		case HOWLINGFIST:
			return howling_cd.ready;
		case FORBIDDENCHAKRA:
			return chakra_cd.ready && chakra == 5;
		case ELIXIRFIELD:
			return elixir_cd.ready;
		case TORNADOKICK:
			return tk_cd.ready && gl.count == 3;
		case RIDDLEOFWIND:
			return row.count > 0;
		case RIDDLEOFFIRE:
			return rof_cd.ready && fists == Fists::FIRE;
		case WINDTACKLE:
			return tackle_cd.ready && fists == Fists::WIND;
		case FIRETACKLE:
			return tackle_cd.ready && fists == Fists::FIRE;
		}
		return false;
	}

	void Mimu::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
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
			gcd_timer.reset(get_gcd_time(), false);
			push_event(gcd_timer.time);
			break;
		case FISTSOFWIND:
			fdafdsff
			break;
		case FISTSOFFIRE:
			adasdasd
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
			gfdgfgsdfg
				dfgdfgfd
				fdgdfg
			rof.reset(ROF_DURATION, 1);
			rof_cd.reset(ROF_CD, false);
			push_event(ROF_DURATION);
			push_event(ROF_CD);
			break;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
	}

	void Mimu::use_damage_action(int action)
	{
		int damage = get_damage(action);
		total_damage += damage;
		history.back().reward += damage;

		bool weaponskill = false;
		bool is_crit = false; // bootshine only

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
			gl.reset(GL_DURATION, std::min(gl.count + 1, 3));
			push_event(GL_DURATION);
			dot_gl = gl.count;
			dot_fof = fists == FISTSOFFIRE;
			dot_twin = twin.count > 0;
			dot_dk = dk.count > 0;
			dot_bro = bro.count > 0;
			dot_rof = rof.count > 0;
			dot_ir = ir.count > 0;
			dot.reset(DOT_DURATION, 1);
			push_event(DOT_DURATION);
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
			break;
		case WINDTACKLE:
			tackle_cd.reset(TACKLE_CD, false);
			row.reset(ROW_DURATION, 1);
			push_event(TACKLE_CD);
			push_event(ROW_DURATION);
			break;
		case FIRETACKLE:
			break;
		}
		if (weaponskill)
		{
			if ((is_crit || prob(rng) < stats.crit_rate + (ir.count > 0 ? IR_CRIT_RATE : 0.0f)) && prob(rng) < MEDITATION_PROC_RATE && chakra < 5)
				chakra++;
			if (bro.count > 0 && prob(rng) < BRO_PROC_RATE && chakra < 5)
				chakra++;
		}
	}
}