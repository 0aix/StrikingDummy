#include "Cube.h"
#include "Logger.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>

#ifdef _DEBUG 
#define DBG(x) x
#else 
#define DBG(x)
#endif

#define CUBE_IDX(x) ((data[x / 8] & (0xF << (4 * (x % 8)))) >> (4 * (x % 8)))
#define SET_CUBE_VAL(x, y) { encoded[x * 6 + CUBE_IDX(x)] = 0.0f; encoded[x * 6 + y] = 1.0f; data[x / 8] = ((data[x / 8] & ~(0xF << (4 * (x % 8)))) | (y << (4 * (x % 8)))); }
#define SET_CUBE_IDX(x, y) SET_CUBE_VAL(x, CUBE_IDX(y))
#define ROTATE(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t) \
{ \
	int tmp = CUBE_IDX(a); \
	if (CUBE_IDX(a) != CUBE_IDX(d)) \
		SET_CUBE_IDX(a, d); \
	if (CUBE_IDX(d) != CUBE_IDX(c)) \
		SET_CUBE_IDX(d, c); \
	if (CUBE_IDX(c) != CUBE_IDX(b)) \
		SET_CUBE_IDX(c, b); \
	if (CUBE_IDX(b) != tmp) \
		SET_CUBE_VAL(b, tmp); \
	tmp = CUBE_IDX(e); \
	if (CUBE_IDX(e) != CUBE_IDX(h)) \
		SET_CUBE_IDX(e, h); \
	if (CUBE_IDX(h) != CUBE_IDX(g)) \
		SET_CUBE_IDX(h, g); \
	if (CUBE_IDX(g) != CUBE_IDX(f)) \
		SET_CUBE_IDX(g, f); \
	if (CUBE_IDX(f) != tmp) \
		SET_CUBE_VAL(f, tmp); \
	tmp = CUBE_IDX(i); \
	if (CUBE_IDX(i) != CUBE_IDX(l)) \
		SET_CUBE_IDX(i, l); \
	if (CUBE_IDX(l) != CUBE_IDX(k)) \
		SET_CUBE_IDX(l, k); \
	if (CUBE_IDX(k) != CUBE_IDX(j)) \
		SET_CUBE_IDX(k, j); \
	if (CUBE_IDX(j) != tmp) \
		SET_CUBE_VAL(j, tmp); \
	tmp = CUBE_IDX(m); \
	if (CUBE_IDX(m) != CUBE_IDX(p)) \
		SET_CUBE_IDX(m, p); \
	if (CUBE_IDX(p) != CUBE_IDX(o)) \
		SET_CUBE_IDX(p, o); \
	if (CUBE_IDX(o) != CUBE_IDX(n)) \
		SET_CUBE_IDX(o, n); \
	if (CUBE_IDX(n) != tmp) \
		SET_CUBE_VAL(n, tmp); \
	tmp = CUBE_IDX(q); \
	if (CUBE_IDX(q) != CUBE_IDX(t)) \
		SET_CUBE_IDX(q, t); \
	if (CUBE_IDX(t) != CUBE_IDX(s)) \
		SET_CUBE_IDX(t, s); \
	if (CUBE_IDX(s) != CUBE_IDX(r)) \
		SET_CUBE_IDX(s, r); \
	if (CUBE_IDX(r) != tmp) \
		SET_CUBE_VAL(r, tmp); \
}

namespace StrikingDummy
{
	Cube::Cube()
	{
		actions.reserve(NUM_ACTIONS);
		for (int i = 0; i < NUM_ACTIONS; i++)
			actions.push_back(i);
		reset();
	}

	void Cube::reset()
	{
		data[0] = 0;
		data[1] = 0x11111111;
		data[2] = 0x22222222;
		data[3] = 0x33333333;
		data[4] = 0x44444444;
		data[5] = 0x55555555;
		memcpy(encoded, encoded_solved, 288 * sizeof(float));
		is_solved = true;

		history.clear();
		update_history();
	}

	void Cube::update()
	{
		update_history();
	}

	void Cube::update_history()
	{
		// state/transition
		if (!history.empty())
		{
			Transition& t = history.back();
			get_state(t.t0);
			if (is_solved)
				t.c0 = true;
		}
		history.emplace_back();
		Transition& t = history.back();
		get_state(t.t1);
		if (is_solved)
			t.c1 = true;
	}

	void Cube::use_action(int action)
	{
		history.back().action = action ^ 1;
		switch (action)
		{
		case 0:
			// F
			ROTATE(0, 2, 7, 5, 1, 4, 6, 3, 8, 42, 31, 37, 11, 41, 28, 38, 13, 40, 26, 39)
			break;
		case 1:
			// F'
			ROTATE(0, 5, 7, 2, 1, 3, 6, 4, 8, 37, 31, 42, 11, 38, 28, 41, 13, 39, 26, 40)
			break;
		case 2:
			// R
			ROTATE(8, 10, 15, 13, 9, 12, 14, 11, 16, 47, 7, 39, 19, 44, 4, 36, 21, 42, 2, 34)
			break;
		case 3:
			// R'
			ROTATE(8, 13, 15, 10, 9, 11, 14, 12, 16, 39, 7, 47, 19, 36, 4, 44, 21, 34, 2, 42)
			break;
		case 4:
			// B
			ROTATE(16, 18, 23, 21, 17, 20, 22, 19, 24, 45, 15, 34, 27, 46, 12, 33, 29, 47, 10, 32)
			break;
		case 5:
			// B'
			ROTATE(16, 21, 23, 18, 17, 19, 22, 20, 24, 34, 15, 45, 27, 33, 12, 46, 29, 32, 10, 47)
			break;
		case 6:
			// L
			ROTATE(24, 26, 31, 29, 25, 28, 30, 27, 0, 40, 23, 32, 3, 43, 20, 35, 5, 45, 18, 37)
			break;
		case 7:
			// L'
			ROTATE(24, 29, 31, 26, 25, 27, 30, 28, 0, 32, 23, 40, 3, 35, 20, 43, 5, 37, 18, 45)
			break;
		case 8:
			// U
			ROTATE(32, 34, 39, 37, 33, 36, 38, 35, 10, 2, 26, 18, 9, 1, 25, 17, 8, 0, 24, 16)
			break;
		case 9:
			// U'
			ROTATE(32, 37, 39, 34, 33, 35, 38, 36, 10, 18, 26, 2, 9, 17, 25, 1, 8, 16, 24, 0)
			break;
		case 10:
			// D
			ROTATE(40, 42, 47, 45, 41, 44, 46, 43, 13, 21, 29, 5, 14, 22, 30, 6, 15, 23, 31, 7)
			break;
		case 11:
			// D'
			ROTATE(40, 45, 47, 42, 41, 43, 46, 44, 13, 5, 29, 21, 14, 6, 30, 22, 15, 7, 31, 23)
			break;
		}
		is_solved = data[0] == 0 && data[1] == 0x11111111 && data[2] == 0x22222222 && data[3] == 0x33333333 && data[4] == 0x44444444 && data[5] == 0x55555555;
	}

	void Cube::get_state(float* state)
	{
		memcpy(state, encoded, 288 * sizeof(float));
	}

	void Cube::get_debug_state(int* data)
	{
		memset(data, 0, 6 * sizeof(int));

		for (int i = 0; i < 48; i++)
		{
			float sum = 0.0f;
			for (int j = 0; j < 6; j++)
				sum += encoded[i * 6 + j];
			assert(sum == 1.0f);
			for (int j = 0; j < 6; j++)
				if (encoded[i * 6 + j] == 1.0f)
					SET_CUBE_VAL(i, j);
		}
	}
}