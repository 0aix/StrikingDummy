#pragma once
#include <queue>

namespace StrikingDummy
{
	struct Job;
	struct Event;
	enum EVENT_TYPE;

	struct Timeline
	{
		std::priority_queue<Event*> events;
		double time;

		Event* next_event();
	};

	struct Job
	{
		virtual void update(double elapsed) = 0;
	};

	struct BlackMage : public Job
	{
		// Assume only in UI3 and AF3
		const int MAX_MP = 15480;
		const int MP_PER_TICK = 9597; // 62% per tick in UI3
		const int CONVERT_MP = 4644;
		
		const double BASE_GCD = 2.50;
		const double III_GCD = 3.50;
		const double IV_GCD = 2.80;
		const double FAST_GCD = 1.75;

		const double FOUL_TIMER = 30.0;
		const double ENO_DURATION = 13.0;
		const double TRIPLE_DURATION = 15.0;
		const double SHARP_DURATION = 15.0;
		const double FS_DURATION = 18.0;
		const double TC_DURATION = 18.0;
		const double LL_DURATION = 30.0;

		// Assume not using B1 or Flare
		const double F1_POTENCY = 180.0;
		const double F3_POTENCY = 240.0;
		const double F4_POTENCY = 300.0;
		const double B3_POTENCY = 240.0;
		const double B4_POTENCY = 260.0;
		const double T3_POTENCY = 70.0;
		const double T3_DOT_POTENCY = 40.0;
		const double TC_POTENCY = 390.0;
		const double FOUL_POTENCY = 650.0;

		const double ENO_MULTIPLIER = 1.10;
		const double AF1_MULTIPLIER = 1.40; // Tranpose + F3P
		const double AF3_MULTIPLIER = 1.80;
		const double AF3UI3_MULTIPLIER = 0.70;

		// MP costs and multipliers 0.25/0.50
		const int F1_MP_COST = 1200;
		const int F3_MP_COST = 2400;
		const int F4_MP_COST = 1200;
		const int B3_MP_COST = 1440;
		const int B4_MP_COST = 960;
		const int T3_MP_COST = 1920;

		int mp = MAX_MP;
		int umbral_hearts = 0;

		double base_gcd = BASE_GCD;
		double iii_gcd = III_GCD;
		double iv_gcd = IV_GCD;
		double fast_gcd = FAST_GCD;
		double ll_base_gcd;
		double ll_iii_gcd;
		double ll_iv_gcd;
		double ll_fast_gcd;

		double foul_timer;
		double eno_duration;
		double triple_duration;
		double sharp_duration;
		double fs_duration;
		double tc_duration;

		bool swift_proc;
		bool sharp_proc;
		int triple_procs;
		bool foul_proc;
		bool fs_proc;
		bool tc_proc;

		bool casting;
		double gcd_to_cast; // amount of time until next cast..?

		double crit;
		double dh;
		double det;
		double sps;
		double stat;
		double wep;

		void calculate_stats();
		void update(double elapsed);

		//void cast(int skill);
	};

	struct Event
	{
		double time;
		EVENT_TYPE type;

		Event(EVENT_TYPE type) : type(type) {}

		bool operator <(const Event& rhs) const
		{
			if (time == rhs.time)
				return type > rhs.type;
			return time > rhs.time;
		}
	};

	struct BlackMageRotation
	{
		virtual void start(BlackMage* blm) = 0;
	};

	struct BalanceRotation : public BlackMageRotation
	{
		Timeline* timeline;
		BlackMage* blm;

		void start(BlackMage* blm);
	};
}