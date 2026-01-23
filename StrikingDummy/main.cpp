#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "BlackMage.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <string>

#define BLACKMAGE

StrikingDummy::Stats load_stats(const char*);
void read_stat(std::ifstream& file, float& value);

int main(int argc, char *argv[])
{
	StrikingDummy::Stats stats = load_stats(argc > 1 ? argv[1] : "stats.txt");
	StrikingDummy::BlackMage blm(stats, StrikingDummy::BlackMage::Opener::GAUGE);
	StrikingDummy::TrainingDummy dummy(blm);
	StrikingDummy::StrikingDummy practice(blm);
	dummy.train();
	//dummy.trace();
	//practice.start();
}

StrikingDummy::Stats load_stats(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw 234;

    StrikingDummy::Stats stats;
    read_stat(file, stats.flat_atk);
    read_stat(file, stats.base_atk_per);
    read_stat(file, stats.refined_atk);
    read_stat(file, stats.str);
    read_stat(file, stats.base_str_per);
    read_stat(file, stats.crit);
    read_stat(file, stats.haste);
    read_stat(file, stats.luck);
    read_stat(file, stats.mastery);
    read_stat(file, stats.vers);
    read_stat(file, stats.base_atk_spd);
    read_stat(file, stats.base_crit_multi);
    read_stat(file, stats.base_luck_multi);
    read_stat(file, stats.base_armor_pen);
    read_stat(file, stats.base_ele_stat);
    read_stat(file, stats.base_serum_stat);
    read_stat(file, stats.base_dmg);

    return stats;
}

void read_stat(std::ifstream& file, float& value)
{
    std::string label;
    // Read the entire label (e.g., "Attack") until the colon
    if (std::getline(file, label, ':'))
    {
        file >> value;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}