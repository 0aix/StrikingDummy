#include "Solver.h"
#include "Model.h"
#include <queue>

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
	Solver::Solver(Model& model) : model(model)
	{

	}

	int Solver::solve(int* data, float* encoded)
	{
		std::vector<State> states;
		std::vector<State> bfs;
		std::vector<Tuple> tuples;
		tuples.reserve(MAX_BREADTH * 12);

		states.emplace_back(data, encoded);
		if (states.back().is_solved())
			return 0;

		for (int step = 1; step <= MAX_STEPS; step++)
		{
			// take what's in states and put it in a batch computation
			for (int i = 0; i < states.size(); i++)
				memcpy(&model.X0[i * STATE_SIZE], &states[i].encoded, sizeof(float) * STATE_SIZE);
			int temp = model.batch_size;
			model.batch_size = states.size();
			float* Q = model.batch_compute();
			model.batch_size = temp;

			// then populate tuples heap using batch computation
			for (int i = 0; i < states.size(); i++)
				for (int j = 0; j < 12; j++)
					tuples.emplace_back(i, j, Q[i * 12 + j]);

			// then populate bfs using tuples and using the accompanying actions to generate new states; check if any are solved while doing this
			if (tuples.size() <= MAX_BREADTH)
			{
				bfs.reserve(tuples.size());
				for (int i = 0; i < tuples.size(); i++)
				{
					bfs.emplace_back(states[tuples[i].index], tuples[i].action);
					if (bfs.back().is_solved())
						return step;
				}
			}
			else
			{
				std::make_heap(tuples.begin(), tuples.end());
				bfs.reserve(MAX_BREADTH);
				for (int i = 0; i < MAX_BREADTH; i++)
				{
					bfs.emplace_back(states[tuples.front().index], tuples.front().action);
					if (bfs.back().is_solved())
						return step;
					std::pop_heap(tuples.begin(), tuples.end());
					tuples.pop_back();
				}
			}
			tuples.clear();
			
			// set states to bfs
			states = std::move(bfs);
		}

		return -1;
	}

	State::State(int* data, float* encoded)
	{
		memcpy(this->data, data, sizeof(this->data));
		memcpy(this->encoded, encoded, sizeof(this->encoded));
	}

	State::State(State& state, int rotate)
	{
		memcpy(data, state.data, sizeof(data));
		memcpy(encoded, state.encoded, sizeof(encoded));
		switch (rotate)
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
	}
}