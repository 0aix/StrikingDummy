#include "StrikingDummy.h"
#include <assert.h>
#include <chrono>
#include <vector>
#include <random>
#include <execution>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace StrikingDummy
{
	using score = int (BalanceRotation::*)();

	score scores[] =
	{
		&BalanceRotation::action_none,
		&BalanceRotation::action_b1,
		&BalanceRotation::action_b3,
		&BalanceRotation::action_b4,
		&BalanceRotation::action_f1,
		&BalanceRotation::action_f3,
		&BalanceRotation::action_f4,
		&BalanceRotation::action_t3,
		&BalanceRotation::action_foul,
		&BalanceRotation::action_swift,
		&BalanceRotation::action_triple,
		&BalanceRotation::action_sharp,
		&BalanceRotation::action_ll,
		&BalanceRotation::action_convert,
		&BalanceRotation::action_eno
	};

	std::vector<int> weights;

	BalanceRotation::BalanceRotation()
	{

	}

	int BalanceRotation::choose_action(std::vector<int>& actions)
	{
		std::transform(actions.begin(), actions.end(), std::back_inserter(weights), [this](int i) { return (this->*scores[i])(); });
		auto max_iter = std::max_element(weights.begin(), weights.end());
		int index = 0;
		if (*max_iter > 0)
			index = std::distance(weights.begin(), max_iter);
		weights.clear();
		return actions[index];
	}

	int BalanceRotation::action_none()
	{
		return 0;
	}

	int BalanceRotation::action_b1()
	{
		return 0;
	}

	int BalanceRotation::action_b3()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->element == BlackMage::Element::NE)
			return 99999;
		if (blm->element == BlackMage::Element::AF)
		{
			if (blm->can_use_action(BlackMage::Action::B3) && !blm->can_use_action(BlackMage::Action::F1))
			{
				return 250;
			}
		}
		return 0;
	}

	int BalanceRotation::action_b4()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->can_use_action(BlackMage::Action::B4))
		{
			if (blm->umbral_hearts != 3)
				return 100;
		}
		return 0;
	}

	int BalanceRotation::action_f1()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->element == BlackMage::Element::AF && blm->can_use_action(BlackMage::Action::F1))
		{
			// hack for now..
			int f4_cast_lock = blm->get_lock_time(BlackMage::Action::F4);
			if (f4_cast_lock + blm->get_cast_time(BlackMage::Action::F1) >= blm->gauge.time)
				return 250;
		}
		return 0;
	}

	int BalanceRotation::action_f3()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->can_use_action(BlackMage::Action::F3))
		{
			if (blm->element == BlackMage::Element::UI)
			{
				if (blm->umbral_hearts == 3)
					return 200;
			}
			else if (blm->element == BlackMage::Element::AF)
			{
				if (blm->fs_proc.count > 0 && blm->mp < 2400)
					return 300;
			}
		}
		return 0;
	}

	int BalanceRotation::action_f4()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->can_use_action(BlackMage::Action::F4))
		{
			return 200;
		}
		return 0;
	}

	int BalanceRotation::action_t3()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->can_use_action(BlackMage::Action::T3))
			if (blm->element == BlackMage::Element::UI && blm->dot.time < 600)
				return 150;
		return 0;
	}

	int BalanceRotation::action_foul()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->element == BlackMage::Element::UI && blm->can_use_action(BlackMage::Action::FOUL))
			return 200;
		return 0;
	}

	int BalanceRotation::action_swift()
	{
		return 0;
	}

	int BalanceRotation::action_triple()
	{
		return 0;
	}

	int BalanceRotation::action_sharp()
	{
		return 0;
	}

	int BalanceRotation::action_ll()
	{
		return 0;
	}

	int BalanceRotation::action_convert()
	{
		return 0;
	}

	int StrikingDummy::BalanceRotation::action_eno()
	{
		BlackMage* blm = (BlackMage*)job;
		if (blm->element != BlackMage::Element::NE && !blm->enochian)
			return 99999;
		return 0;
	}
}