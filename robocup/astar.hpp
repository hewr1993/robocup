#ifndef __ASTAR_HPP
#define __ASTAR_HPP

#include "robomap.hpp"
#include <vector>

namespace astar{
	struct ASPoint
	{
		int col;
		int row;
		ASPoint()
 		{ }
		ASPoint(int c, int r) : col(c), row(r)
 		{ }
	};

	enum ASDirect
	{
		LEFT = 1,
		RIGHT = 3,
		UP = 0,
		DOWN = 2,
		ERROR = -1
	};

	const ASDirect ASDirection[] = {UP, LEFT, DOWN, RIGHT};

	ASDirect asDirectTo(ASPoint ptFrom, ASPoint ptTo, ASDirect face = UP);

	std::vector<ASPoint> asFindPath(ASPoint loc);
	ASDirect asFindDirect(ASPoint loc, ASDirect face = UP);
};

#endif
