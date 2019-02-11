#include "Samurai.h"
#include <assert.h>

namespace StrikingDummy
{
	Samurai::Samurai(Stats& stats) :
		Job(stats, SAM_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD)))),
		iai_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * IAI_GCD)))),
		shifu_base_gcd(lround(floor(0.1f * floor(0.90f * floor(this->stats.ss_multiplier * BASE_GCD))))),
		shifu_iai_gcd(lround(floor(0.1f * floor(0.90f * floor(this->stats.ss_multiplier * IAI_GCD))))),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay)))),
		shifu_auto_gcd(lround(floor(0.1f * floor(0.90f * floor(1000.0f * this->stats.auto_delay)))))
	{
		actions.reserve(NUM_ACTIONS);
		reset();
	}

	void Samurai::reset()
	{
		timeline = {};

		kenki = 0;
		setsu = false;
		getsu = false;
		ka = false;

		dot_timer.reset(tick(rng), false);
		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);

		// buffs
		jinpu_combo.reset(0, 0);
		shifu_combo.reset(0, 0);
		gekko_combo.reset(0, 0);
		kasha_combo.reset(0, 0);
		yukikaze_combo.reset(0, 0);
		jinpu.reset(0, 0);
		shifu.reset(0, 0);
		yukikaze.reset(0, 0);
		meikyo.reset(0, 0);
		kaiten.reset(0, 0);
		dot.reset(0, 0);

		dot_jinpu = false;
		dot_yukikaze = false;
		dot_kaiten = false;

		// cooldowns
		meikyo_cd.reset(0, true);
		kaiten_cd.reset(0, true);
		shinten_cd.reset(0, true);
		hagakure_cd.reset(0, true);
		guren_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = NONE;

		// metrics
		total_damage = 0.0f;
		midare_count = 0;

		history.clear();

		update_history();
	}

	void Samurai::update(int elapsed)
	{
		assert(elapsed > 0);

		// server ticks
		dot_timer.update(elapsed);
		auto_timer.update(elapsed);

		// buffs
		jinpu_combo.update(elapsed);
		shifu_combo.update(elapsed);
		gekko_combo.update(elapsed);
		kasha_combo.update(elapsed);
		yukikaze_combo.update(elapsed);
		jinpu.update(elapsed);
		shifu.update(elapsed);
		yukikaze.update(elapsed);
		meikyo.update(elapsed);
		kaiten.update(elapsed);
		dot.update(elapsed);

		// cooldowns
		meikyo_cd.update(elapsed);
		kaiten_cd.update(elapsed);
		shinten_cd.update(elapsed);
		hagakure_cd.update(elapsed);
		guren_cd.update(elapsed);

		// actions
		gcd_timer.update(elapsed);
		cast_timer.update(elapsed);
		action_timer.update(elapsed);

		// 
		if (dot_timer.ready)
			update_dot();
		if (auto_timer.ready)
			update_auto();
		if (cast_timer.ready)
			end_action();

		update_history();
	}

	void Samurai::update_history()
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

	void Samurai::update_dot()
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

	void Samurai::update_auto()
	{
		float damage = get_auto_damage();
		total_damage += damage;
		history.back().reward += damage;
		if (shifu.count > 0)
			auto_timer.reset(shifu_auto_gcd, false);
		else
			auto_timer.reset(auto_gcd, false);
		push_event(auto_timer.time);
	}

	int Samurai::get_gcd_time() const
	{
		return shifu.count > 0 ? shifu_base_gcd : base_gcd;
	}

	bool Samurai::can_use_action(int action) const
	{
		switch (action)
		{
		case NONE:
			return !gcd_timer.ready;
		case HAKAZE:
			return gcd_timer.ready && meikyo.count == 0 &&
				jinpu_combo.count == 0 &&
				shifu_combo.count == 0 &&
				gekko_combo.count == 0 &&
				kasha_combo.count == 0 &&
				yukikaze_combo.count == 0;
		case JINPU:
			return gcd_timer.ready && (jinpu_combo.count > 0 || meikyo.count > 0);
		case SHIFU:
			return gcd_timer.ready && (shifu_combo.count > 0 || meikyo.count > 0);
		case GEKKO:
			return gcd_timer.ready && (gekko_combo.count > 0 || meikyo.count > 0);
		case KASHA:
			return gcd_timer.ready && (kasha_combo.count > 0 || meikyo.count > 0);
		case YUKIKAZE:
			return gcd_timer.ready && (yukikaze_combo.count > 0 || meikyo.count > 0);
		case HIGANBANA:
			return gcd_timer.ready && ((int)setsu + (int)getsu + (int)ka) == 1;
		case TENKA:
			return gcd_timer.ready && ((int)setsu + (int)getsu + (int)ka) == 2;
		case MIDARE:
			return gcd_timer.ready && ((int)setsu + (int)getsu + (int)ka) == 3;
		case MEIKYO:
			return meikyo_cd.ready;
		case KAITEN:
			return kaiten_cd.ready && kenki >= KAITEN_COST;
		case SHINTEN:
			return shinten_cd.ready && kenki >= SHINTEN_COST;
		case GUREN:
			return guren_cd.ready && kenki >= GUREN_COST;
		case HAGAKURE:
			return hagakure_cd.ready && (setsu || getsu || ka);
		}
		return false;
	}

	void Samurai::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
			return;
		case HAKAZE:
		case JINPU:
		case SHIFU:
		case GEKKO:
		case KASHA:
		case YUKIKAZE:
		case SHINTEN:
		case GUREN:
			use_damage_action(action);
			break;
		case HIGANBANA:
		case TENKA:
		case MIDARE:
			if (shifu_iai_gcd < shifu.time)
			{
				gcd_timer.reset(shifu_base_gcd, false);
				cast_timer.reset(shifu_iai_gcd, false);
				action_timer.reset(shifu_iai_gcd + ACTION_TAX, false);
			}
			else
			{
				gcd_timer.reset(base_gcd, false);
				cast_timer.reset(iai_gcd, false);
				action_timer.reset(iai_gcd + ACTION_TAX, false);
			}
			if (action == MIDARE)
				midare_count++;
			casting = action;
			push_event(gcd_timer.time);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			return;
		case MEIKYO:
			meikyo.reset(MEIKYO_DURATION, 3);
			meikyo_cd.reset(MEIKYO_CD, false);
			push_event(MEIKYO_DURATION);
			push_event(MEIKYO_CD);
			break;
		case KAITEN:
			kenki -= KAITEN_COST;
			kaiten.reset(KAITEN_DURATION, 1);
			kaiten_cd.reset(KAITEN_CD, false);
			push_event(KAITEN_DURATION);
			push_event(KAITEN_CD);
			break;
		case HAGAKURE:
			kenki = std::min(MAX_KENKI, kenki + 20 * ((int)setsu + (int)getsu + (int)ka));
			setsu = false;
			getsu = false;
			ka = false;
			hagakure_cd.reset(HAGAKURE_CD, false);
			push_event(HAGAKURE_CD);
			break;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
	}

	void Samurai::use_damage_action(int action)
	{
		float damage = get_damage(action);
		total_damage += damage;
		history.back().reward += damage;

		bool weaponskill = false;
		int gcd_time = get_gcd_time();

		switch (action)
		{
		case HAKAZE:
			kenki = std::min(MAX_KENKI, kenki + 5);
			if (meikyo.count > 0)
				if (--meikyo.count == 0)
					meikyo.reset(0, 0);
			if (meikyo.count == 0)
			{
				jinpu_combo.reset(COMBO_DURATION, 1);
				shifu_combo.reset(COMBO_DURATION, 1);
				gekko_combo.reset(0, 0);
				kasha_combo.reset(0, 0);
				yukikaze_combo.reset(COMBO_DURATION, 1);
				push_event(COMBO_DURATION);
			}
			weaponskill = true;
			break;
		case JINPU:
			if (meikyo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				jinpu.reset(JINPU_DURATION, 1);
				push_event(JINPU_DURATION);
				if (--meikyo.count == 0)
				{
					meikyo.reset(0, 0);
					jinpu_combo.reset(0, 0);
					shifu_combo.reset(0, 0);
					gekko_combo.reset(0, 0);
					kasha_combo.reset(0, 0);
					yukikaze_combo.reset(0, 0);
				}
			}
			else if (jinpu_combo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				jinpu.reset(JINPU_DURATION, 1);
				jinpu_combo.reset(0, 0);
				shifu_combo.reset(0, 0);
				gekko_combo.reset(COMBO_DURATION, 1);
				kasha_combo.reset(0, 0);
				yukikaze_combo.reset(0, 0);
				push_event(JINPU_DURATION);
				push_event(COMBO_DURATION);
			}
			weaponskill = true;
			break;
		case SHIFU:
			if (meikyo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				shifu.reset(SHIFU_DURATION, 1);
				push_event(SHIFU_DURATION);
				if (--meikyo.count == 0)
				{
					meikyo.reset(0, 0);
					jinpu_combo.reset(0, 0);
					shifu_combo.reset(0, 0);
					gekko_combo.reset(0, 0);
					kasha_combo.reset(0, 0);
					yukikaze_combo.reset(0, 0);
				}
			}
			else if (shifu_combo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				shifu.reset(SHIFU_DURATION, 1);
				jinpu_combo.reset(0, 0);
				shifu_combo.reset(0, 0);
				gekko_combo.reset(0, 0);
				kasha_combo.reset(COMBO_DURATION, 1);
				yukikaze_combo.reset(0, 0);
				push_event(JINPU_DURATION);
				push_event(COMBO_DURATION);
			}
			weaponskill = true;
			break;
		case GEKKO:
			if (meikyo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				getsu = true;
				if (--meikyo.count == 0)
				{
					meikyo.reset(0, 0);
					jinpu_combo.reset(0, 0);
					shifu_combo.reset(0, 0);
					gekko_combo.reset(0, 0);
					kasha_combo.reset(0, 0);
					yukikaze_combo.reset(0, 0);
				}
			}
			else if (gekko_combo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				getsu = true;
				jinpu_combo.reset(0, 0);
				shifu_combo.reset(0, 0);
				gekko_combo.reset(0, 0);
				kasha_combo.reset(0, 0);
				yukikaze_combo.reset(0, 0);
			}
			weaponskill = true;
			break;
		case KASHA:
			if (meikyo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				ka = true;
				if (--meikyo.count == 0)
				{
					meikyo.reset(0, 0);
					jinpu_combo.reset(0, 0);
					shifu_combo.reset(0, 0);
					gekko_combo.reset(0, 0);
					kasha_combo.reset(0, 0);
					yukikaze_combo.reset(0, 0);
				}
			}
			else if (kasha_combo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				ka = true;
				jinpu_combo.reset(0, 0);
				shifu_combo.reset(0, 0);
				gekko_combo.reset(0, 0);
				kasha_combo.reset(0, 0);
				yukikaze_combo.reset(0, 0);
			}
			weaponskill = true;
			break;
		case YUKIKAZE:
			if (meikyo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				setsu = true;
				if (--meikyo.count == 0)
				{
					meikyo.reset(0, 0);
					jinpu_combo.reset(0, 0);
					shifu_combo.reset(0, 0);
					gekko_combo.reset(0, 0);
					kasha_combo.reset(0, 0);
					yukikaze_combo.reset(0, 0);
				}
			}
			else if (yukikaze_combo.count > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				setsu = true;
				jinpu_combo.reset(0, 0);
				shifu_combo.reset(0, 0);
				gekko_combo.reset(0, 0);
				kasha_combo.reset(0, 0);
				yukikaze_combo.reset(0, 0);
			}
			weaponskill = true;
			break;
		case SHINTEN:
			kenki -= SHINTEN_COST;
			shinten_cd.reset(SHINTEN_CD, false);
			push_event(SHINTEN_CD);
			break;
		case GUREN:
			kenki -= GUREN_COST;
			guren_cd.reset(GUREN_CD, false);
			push_event(GUREN_CD);
		}
		if (weaponskill)
		{
			if (kaiten.count > 0)
				kaiten.reset(0, 0);
			gcd_timer.reset(gcd_time, false);
			push_event(gcd_time);
		}
	}

	void Samurai::end_action()
	{
		assert(casting != NONE);

		float damage = get_damage(casting);
		total_damage += damage;
		history.back().reward += damage;

		if (casting == HIGANBANA)
		{
			dot.reset(DOT_DURATION, 1);
			push_event(DOT_DURATION);
			dot_jinpu = jinpu.count > 0;
			dot_yukikaze = yukikaze.count > 0;
			dot_kaiten = kaiten.count > 0;
		}

		setsu = false;
		getsu = false;
		ka = false;
		if (kaiten.count > 0)
			kaiten.reset(0, 0);

		casting = NONE;
		cast_timer.ready = false;
	}

	float Samurai::get_damage(int action) const
	{
		float potency = 0.0f;
		bool weaponskill = false;
		switch (action)
		{
		case HAKAZE:
			potency = HAKAZE_POTENCY;
			weaponskill = true;
			break;
		case JINPU:
			potency = (jinpu_combo.count > 0 || meikyo.count > 0) ? JINPU_COMBO_POTENCY : JINPU_POTENCY;
			weaponskill = true;
			break;
		case SHIFU:
			potency = (shifu_combo.count > 0 || meikyo.count > 0) ? SHIFU_COMBO_POTENCY : SHIFU_POTENCY;
			weaponskill = true;
			break;
		case GEKKO:
			potency = (gekko_combo.count > 0 || meikyo.count > 0) ? GEKKO_COMBO_POTENCY : GEKKO_POTENCY;
			weaponskill = true;
			break;
		case KASHA:
			potency = (kasha_combo.count > 0 || meikyo.count > 0) ? KASHA_COMBO_POTENCY : KASHA_POTENCY;
			weaponskill = true;
			break;
		case YUKIKAZE:
			potency = (yukikaze_combo.count > 0 || meikyo.count > 0) ? YUKIKAZE_COMBO_POTENCY : YUKIKAZE_POTENCY;
			weaponskill = true;
			break;
		case SHINTEN:
			potency = SHINTEN_POTENCY;
			break;
		case GUREN:
			potency = GUREN_POTENCY;
			break;
		case HIGANBANA:
			potency = HIGANBANA_POTENCY;
			weaponskill = true;
			break;
		case TENKA:
			potency = TENKA_POTENCY;
			weaponskill = true;
			break;
		case MIDARE:
			potency = MIDARE_POTENCY;
			weaponskill = true;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		float damage = potency * stats.potency_multiplier *
			((weaponskill && kaiten.count > 0) ? KAITEN_MULTIPLIER : 1.0f) *
			(jinpu.count > 0 ? JINPU_MULTIPLIER : 1.0f) *
			(yukikaze.count > 0 ? YUKIKAZE_MULTIPLIER : 1.0f);
		return damage * stats.expected_multiplier;
	}

	float Samurai::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return HIGANBANA_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier *
			(dot_kaiten ? KAITEN_MULTIPLIER : 1.0f) *
			(dot_jinpu ? JINPU_MULTIPLIER : 1.0f) *
			(dot_yukikaze ? YUKIKAZE_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	float Samurai::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier *
			(jinpu.count > 0 ? JINPU_MULTIPLIER : 1.0f) *
			(yukikaze.count > 0 ? YUKIKAZE_MULTIPLIER : 1.0f) *
			stats.expected_multiplier;
	}

	void Samurai::get_state(float* state)
	{
		state[0] = kenki / (float)MAX_KENKI;
		state[1] = setsu;
		state[2] = getsu;
		state[3] = ka;
		state[4] = jinpu_combo.count > 0 || meikyo.count > 0;
		state[5] = shifu_combo.count > 0 || meikyo.count > 0;
		state[6] = gekko_combo.count > 0 || meikyo.count > 0;
		state[7] = kasha_combo.count > 0 || meikyo.count > 0;
		state[8] = yukikaze_combo.count > 0 || meikyo.count > 0;
		state[9] = jinpu.count > 0;
		state[10] = jinpu.time / (float)JINPU_DURATION;
		state[11] = shifu.count > 0;
		state[12] = shifu.time / (float)SHIFU_DURATION;
		state[13] = yukikaze.count > 0;
		state[14] = yukikaze.time / (float)YUKIKAZE_DURATION;
		state[15] = meikyo.count >= 1;
		state[16] = meikyo.count >= 2;
		state[17] = meikyo.count == 3;
		state[18] = meikyo.time / (float)MEIKYO_DURATION;
		state[19] = kaiten.count > 0;
		state[20] = kaiten.time / (float)KAITEN_DURATION;
		state[21] = dot.count > 0;
		state[22] = dot.time / (float)DOT_DURATION;
		state[23] = dot.count > 0 && dot_jinpu;
		state[24] = dot.count > 0 && dot_yukikaze;
		state[25] = dot.count > 0 && dot_kaiten;
		state[26] = meikyo_cd.ready;
		state[27] = meikyo_cd.time / (float)MEIKYO_CD;
		state[28] = kaiten_cd.ready;
		state[29] = kaiten_cd.time / (float)KAITEN_CD;
		state[30] = shinten_cd.ready;
		state[31] = shinten_cd.time / (float)SHINTEN_CD;
		state[32] = hagakure_cd.ready;
		state[33] = hagakure_cd.time / (float)HAGAKURE_CD;
		state[34] = guren_cd.ready;
		state[35] = guren_cd.time / (float)GUREN_CD;
		state[36] = gcd_timer.ready;
		state[37] = gcd_timer.time / 250.0f;
	}
}