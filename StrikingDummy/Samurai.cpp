#include "Samurai.h"
#include <assert.h>

namespace StrikingDummy
{
	Samurai::Samurai(Stats& stats) :
		Job(stats, SAM_ATTR),
		base_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * BASE_GCD))) * 10),
		iai_gcd(lround(floor(0.1f * floor(this->stats.ss_multiplier * IAI_GCD))) * 10),
		shifu_base_gcd(lround(floor(0.1f * floor(0.87f * floor(this->stats.ss_multiplier * BASE_GCD)))) * 10),
		shifu_iai_gcd(lround(floor(0.1f * floor(0.87f * floor(this->stats.ss_multiplier * IAI_GCD)))) * 10),
		auto_gcd(lround(floor(0.1f * floor(1000.0f * this->stats.auto_delay))) * 10),
		shifu_auto_gcd(lround(floor(0.1f * floor(0.87f * floor(1000.0f * this->stats.auto_delay)))) * 10)
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
		meditation = 0;
		meikyo = 0;
		can_jinpu = false;
		can_shifu = false;
		can_gekko = false;
		can_kasha = false;
		can_yukikaze = false;
		can_kaiten = true;
		can_shinten = true;
		can_tsubame = false;
		kaiten = false;

		dot_timer.reset(tick(rng), false);
		auto_timer.reset(std::uniform_int_distribution<int>(1, auto_gcd)(rng), false);
		timeline.push_event(dot_timer.time);
		timeline.push_event(auto_timer.time);

		// buffs
		jinpu.reset(0, 0);
		shifu.reset(0, 0);
		dot.reset(0, 0);
		pot.reset(0, 0);

		dot_jinpu = false;
		dot_kaiten = false;
		dot_pot = false;

		// cooldowns
		meikyo_cd.reset(0, true);
		hagakure_cd.reset(0, true);
		senei_cd.reset(0, true);
		ikishoten_cd.reset(0, true);
		tsubame_cd.reset(0, true);
		pot_cd.reset(0, true);

		// actions
		gcd_timer.reset(0, true);
		cast_timer.reset(0, false);
		action_timer.reset(0, true);
		casting = NONE;

		// metrics
		total_damage = 0.0f;
		midare_count = 0;
		higanbana_count = 0;
		tsubame_count = 0;
		kaiten_count = 0;
		senei_count = 0;
		shinten_count = 0;
		hagakure_count = 0;
		shoha_count = 0;
		pot_count = 0;

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
		jinpu.update(elapsed);
		shifu.update(elapsed);
		dot.update(elapsed);
		pot.update(elapsed);

		// cooldowns
		meikyo_cd.update(elapsed);
		hagakure_cd.update(elapsed);
		senei_cd.update(elapsed);
		ikishoten_cd.update(elapsed);
		tsubame_cd.update(elapsed);
		pot_cd.update(elapsed);

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
			return gcd_timer.ready && meikyo == 0 &&
				   !can_jinpu && !can_shifu &&
				   !can_gekko && !can_kasha &&
				   !can_yukikaze;
		case JINPU:
			return gcd_timer.ready && (can_jinpu || meikyo > 0);
		case SHIFU:
			return gcd_timer.ready && (can_shifu || meikyo > 0);
		case GEKKO:
			return gcd_timer.ready && (can_gekko || meikyo > 0);
		case KASHA:
			return gcd_timer.ready && (can_kasha || meikyo > 0);
		case YUKIKAZE:
			return gcd_timer.ready && (can_yukikaze || meikyo > 0);
		case HIGANBANA:
			return gcd_timer.ready && ((int)setsu + (int)getsu + (int)ka) == 1;
		case MIDARE:
			return gcd_timer.ready && ((int)setsu + (int)getsu + (int)ka) == 3;
		case TSUBAME:
			return gcd_timer.ready && tsubame_cd.ready && can_tsubame;
		case MEIKYO:
			return meikyo_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case KAITEN:
			return can_kaiten && kenki >= KAITEN_COST && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case SHINTEN:
			return can_shinten && kenki >= SHINTEN_COST && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case SENEI:
			return senei_cd.ready && kenki >= SENEI_COST && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case HAGAKURE:
			return hagakure_cd.ready && (setsu || getsu || ka) && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case IKISHOTEN:
			return ikishoten_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case SHOHA:
			return meditation == MAX_MEDITATION && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
		case POT:
			// let it clip since gcd is really fast
			//return pot_cd.ready && gcd_timer.time >= ANIMATION_LOCK + ACTION_TAX;
			return pot_cd.ready && gcd_timer.time >= POTION_LOCK + ACTION_TAX;
		}
		return false;
	}

	void Samurai::use_action(int action)
	{
		history.back().action = action;
		switch (action)
		{
		case NONE:
			action_timer.reset(gcd_timer.time, false);
			push_event(action_timer.time);
			return;
		case HAKAZE:
		case JINPU:
		case SHIFU:
		case GEKKO:
		case KASHA:
		case YUKIKAZE:
		case SHINTEN:
		case SENEI:
		case SHOHA:
		case TSUBAME:
			if (action == SHINTEN)
				shinten_count++;
			else if (action == SENEI)
				senei_count++;
			else if (action == SHOHA)
				shoha_count++;
			else if (action == TSUBAME)
				tsubame_count++;
			use_damage_action(action);
			break;
		case HIGANBANA:
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
			else if (action == HIGANBANA)
				higanbana_count++;
			casting = action;
			push_event(gcd_timer.time);
			push_event(cast_timer.time);
			push_event(action_timer.time);
			return;
		case MEIKYO:
			meikyo = 3;
			can_jinpu = true;
			can_shifu = true;
			can_gekko = true;
			can_kasha = true;
			can_yukikaze = true;
			meikyo_cd.reset(MEIKYO_CD, false);
			push_event(MEIKYO_CD);
			break;
		case KAITEN:
			kenki -= KAITEN_COST;
			kaiten = true;
			can_kaiten = false;
			kaiten_count++;
			break;
		case HAGAKURE:
			hagakure_count++;
			kenki = std::min(MAX_KENKI, kenki + HAGAKURE_KENKI * ((int)setsu + (int)getsu + (int)ka));
			setsu = false;
			getsu = false;
			ka = false;
			hagakure_cd.reset(HAGAKURE_CD, false);
			push_event(HAGAKURE_CD);
			break;
		case IKISHOTEN:
			kenki = std::min(MAX_KENKI, kenki + IKISHOTEN_KENKI);
			ikishoten_cd.reset(IKISHOTEN_CD, false);
			push_event(IKISHOTEN_CD);
			break;
		case POT:
			pot_count++;
			pot.reset(POT_DURATION, 1);
			pot_cd.reset(POT_CD, false);
			push_event(POT_DURATION);
			push_event(POT_CD);
			action_timer.reset(POTION_LOCK + ACTION_TAX, false);
			push_event(action_timer.time);
			can_tsubame = false;
			return;
		}
		action_timer.reset(ANIMATION_LOCK + ACTION_TAX, false);
		push_event(action_timer.time);
		can_tsubame = false;
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
			can_jinpu = true;
			can_shifu = true;
			can_yukikaze = true;
			can_gekko = false;
			can_kasha = false;
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case JINPU:
			if (meikyo > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				jinpu.reset(JINPU_DURATION, 1);
				push_event(JINPU_DURATION);
				if (--meikyo == 0)
				{
					can_jinpu = false;
					can_shifu = false;
					can_gekko = false;
					can_kasha = false;
					can_yukikaze = false;
				}
			}
			else if (can_jinpu)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				jinpu.reset(JINPU_DURATION, 1);
				can_jinpu = false;
				can_shifu = false;
				can_gekko = true;
				can_kasha = false;
				can_yukikaze = false;
				push_event(JINPU_DURATION);
			}
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case SHIFU:
			if (meikyo > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				shifu.reset(SHIFU_DURATION, 1);
				push_event(SHIFU_DURATION);
				if (--meikyo == 0)
				{
					can_jinpu = false;
					can_shifu = false;
					can_gekko = false;
					can_kasha = false;
					can_yukikaze = false;
				}
			}
			else if (can_shifu)
			{
				kenki = std::min(MAX_KENKI, kenki + 5);
				shifu.reset(SHIFU_DURATION, 1);
				can_jinpu = false;
				can_shifu = false;
				can_gekko = false;
				can_kasha = true;
				can_yukikaze = false;
				push_event(SHIFU_DURATION);
			}
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case GEKKO:
			if (meikyo > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				getsu = true;
				if (--meikyo == 0)
				{
					can_jinpu = false;
					can_shifu = false;
					can_gekko = false;
					can_kasha = false;
					can_yukikaze = false;
				}
			}
			else if (can_gekko)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				getsu = true;
				can_jinpu = false;
				can_shifu = false;
				can_gekko = false;
				can_kasha = false;
				can_yukikaze = false;
			}
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case KASHA:
			if (meikyo > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				ka = true;
				if (--meikyo == 0)
				{
					can_jinpu = false;
					can_shifu = false;
					can_gekko = false;
					can_kasha = false;
					can_yukikaze = false;
				}
			}
			else if (can_kasha)
			{
				kenki = std::min(MAX_KENKI, kenki + 10);
				ka = true;
				can_jinpu = false;
				can_shifu = false;
				can_gekko = false;
				can_kasha = false;
				can_yukikaze = false;
			}
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case YUKIKAZE:
			if (meikyo > 0)
			{
				kenki = std::min(MAX_KENKI, kenki + 15);
				setsu = true;
				if (--meikyo == 0)
				{
					can_jinpu = false;
					can_shifu = false;
					can_gekko = false;
					can_kasha = false;
					can_yukikaze = false;
				}
			}
			else if (can_yukikaze)
			{
				kenki = std::min(MAX_KENKI, kenki + 15);
				setsu = true;
				can_jinpu = false;
				can_shifu = false;
				can_gekko = false;
				can_kasha = false;
				can_yukikaze = false;
			}
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		case SHINTEN:
			kenki -= SHINTEN_COST;
			can_shinten = false;
			break;
		case SENEI:
			kenki -= SENEI_COST;
			senei_cd.reset(SENEI_CD, false);
			push_event(SENEI_CD);
			break;
		case SHOHA:
			meditation = 0;
			break;
		case TSUBAME:
			meditation = std::min(MAX_MEDITATION, meditation + 1);
			tsubame_cd.reset(TSUBAME_CD, false);
			push_event(TSUBAME_CD);
			can_kaiten = true;
			can_shinten = true;
			weaponskill = true;
			break;
		}
		if (weaponskill)
		{
			if (kaiten)
				kaiten = false;
			gcd_timer.reset(gcd_time, false);
			push_event(gcd_time);
		}
		can_tsubame = false;
	}

	void Samurai::end_action()
	{
		assert(casting != NONE);

		float damage = get_damage(casting);
		total_damage += damage;
		history.back().reward += damage;

		switch (casting)
		{
		case HIGANBANA:
			dot.reset(DOT_DURATION, 1);
			push_event(DOT_DURATION);
			dot_jinpu = jinpu.count > 0;
			dot_kaiten = kaiten;
			dot_pot = pot.count > 0;
			break;
		case MIDARE:
			can_tsubame = true;
			break;
		}

		meditation = std::min(MAX_MEDITATION, meditation + 1);

		setsu = false;
		getsu = false;
		ka = false;
		can_kaiten = true;
		can_shinten = true;
		kaiten = false;
		
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
			potency = JINPU_POTENCY;
			weaponskill = true;
			break;
		case SHIFU:
			potency = SHIFU_POTENCY;
			weaponskill = true;
			break;
		case GEKKO:
			potency = GEKKO_POTENCY;
			weaponskill = true;
			break;
		case KASHA:
			potency = KASHA_POTENCY;
			weaponskill = true;
			break;
		case YUKIKAZE:
			potency = YUKIKAZE_POTENCY;
			weaponskill = true;
			break;
		case SHINTEN:
			potency = SHINTEN_POTENCY;
			break;
		case SENEI:
			potency = SENEI_POTENCY;
			break;
		case HIGANBANA:
			potency = HIGANBANA_POTENCY;
			weaponskill = true;
			break;
		case MIDARE:
			potency = MIDARE_POTENCY;
			weaponskill = true;
			break;
		case TSUBAME:
			potency = TSUBAME_POTENCY;
			break;
		case SHOHA:
			potency = SHOHA_POTENCY;
			break;
		}
		// floor(ptc * wd * ap * det * traits) * chr | * dhr | * rand(.95, 1.05) | ...
		return potency * stats.potency_multiplier * ((weaponskill && kaiten) ? KAITEN_MULTIPLIER : 1.0f) * (jinpu.count > 0 ? JINPU_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * stats.expected_multiplier;
	}

	float Samurai::get_dot_damage() const
	{
		// floor(ptc * wd * ap * det * traits) * ss | * rand(.95, 1.05) | * chr | * dhr | ...
		return HIGANBANA_DOT_POTENCY * stats.potency_multiplier * stats.dot_multiplier * (dot_kaiten ? KAITEN_MULTIPLIER : 1.0f) * (dot_jinpu ? JINPU_MULTIPLIER : 1.0f) * (dot_pot ? stats.pot_multiplier : 1.0f) * stats.expected_multiplier;
	}

	float Samurai::get_auto_damage() const
	{
		// floor(ptc * aa * ap * det * traits) * ss | * chr | * dhr | * rand(.95, 1.05) | ...
		return stats.aa_multiplier * stats.dot_multiplier * (jinpu.count > 0 ? JINPU_MULTIPLIER : 1.0f) * (pot.count > 0 ? stats.pot_multiplier : 1.0f) * stats.expected_multiplier;
	}

	void Samurai::get_state(float* state)
	{
		state[0] = kenki / (float)MAX_KENKI;
		state[1] = setsu;
		state[2] = getsu;
		state[3] = ka;
		state[4] = meditation / 3.0f;
		state[5] = meikyo >= 1;
		state[6] = meikyo >= 2;
		state[7] = meikyo == 3;
		state[8] = can_jinpu;
		state[9] = can_shifu;
		state[10] = can_gekko;
		state[11] = can_kasha;
		state[12] = can_yukikaze;
		state[13] = can_kaiten;
		state[14] = can_shinten;
		state[15] = can_tsubame;
		state[16] = kaiten;
		state[17] = jinpu.count > 0;
		state[18] = jinpu.time / (float)JINPU_DURATION;
		state[19] = shifu.count > 0;
		state[20] = shifu.time / (float)SHIFU_DURATION;
		state[21] = pot.count > 0;
		state[22] = pot.time / (float)POT_DURATION;
		state[23] = dot.count > 0;
		state[24] = dot.time / (float)DOT_DURATION;
		state[25] = dot.count > 0 && dot_jinpu;
		state[26] = dot.count > 0 && dot_pot;
		state[27] = dot.count > 0 && dot_kaiten;
		state[28] = meikyo_cd.ready;
		state[29] = meikyo_cd.time / (float)MEIKYO_CD;
		state[30] = hagakure_cd.ready;
		state[31] = hagakure_cd.time / (float)HAGAKURE_CD;
		state[32] = senei_cd.ready;
		state[33] = senei_cd.time / (float)SENEI_CD;
		state[34] = ikishoten_cd.ready;
		state[35] = ikishoten_cd.time / (float)IKISHOTEN_CD;
		state[36] = tsubame_cd.ready;
		state[37] = tsubame_cd.time / (float)TSUBAME_CD;
		state[38] = pot_cd.ready;
		state[39] = pot_cd.time / (float)POT_CD;
		state[40] = gcd_timer.ready;
		state[41] = gcd_timer.time / (BASE_GCD * 1000.0f);
		state[42] = ((int)setsu + (int)getsu + (int)ka) / 3.0f;
	}

	std::string Samurai::get_info()
	{
		return "\n";
	}
}