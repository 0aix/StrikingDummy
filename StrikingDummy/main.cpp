#include "StrikingDummy.h"
#include "TrainingDummy.h"
#include "Cube.h"
#include "Logger.h"
#include <iostream>

#define BLACKMAGE

int main()
{
	StrikingDummy::Cube cube;
	StrikingDummy::TrainingDummy dummy(cube);
	dummy.train();

	// L' B R2 D' L' D L U' D2 L2 B' R U' B2 R' U' D' B R' U' R2 L2 D2 F R
	//int moves[] = { 7, 4, 2, 2, 11, 7, 10, 6, 9, 10, 10, 6, 6, 5, 2, 9, 4, 4, 3, 9, 11, 4, 3, 9, 2, 2, 6, 6, 10, 10, 0, 2 };
}