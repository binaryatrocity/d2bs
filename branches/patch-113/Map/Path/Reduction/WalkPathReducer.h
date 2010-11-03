#pragma once

#include "PathReducer.h"
#include "../Path.h"
#include "../../D2Map.h"

namespace Mapping
{
namespace Pathing
{
namespace Reducing
{

#pragma warning ( disable: 4512 )

class WalkPathReducer : public PathReducer
{
private:
	D2Map& map;
	Distance distance;
	int range;

	inline void swap(int* x, int* y) { int t = *x; *x = *y; *y = t; }

	void Line(Point start, Point end, PointList& list)
	{
		int x0 = start.first, y0 = start.second,
			x1 = end.first, y1 = end.second;
		bool steep = abs(y1 - y0) > abs(x1 - x0);
		if(steep)
		{
			swap(&x0, &y0);
			swap(&x1, &y1);
		}
		if(x0 > x1)
		{
			swap(&x0, &x1);
			swap(&y0, &y1);
		}
		int dx = x1 - x0, dy = abs(y1 - y0),
			error = dx/2,
			ystep, y = y0;

		if(y0 < y1) ystep = 1;
		else ystep = -1;

		for(int x = x0; x < x1; x++)
		{
			if(steep) list.push_back(Point(y, x));
			else list.push_back(Point(x, y));

			error -= dy;
			if(error < 0)
			{
				y += ystep;
				error += dx;
			}
		}
	}

public:
	WalkPathReducer(D2Map& m, Distance d, int _range = 20) : map(m), distance(d), range(_range*10) {}

	void Reduce(PointList const & in, PointList& out, bool abs)
	{
		PointList::const_iterator it = in.begin(), end = in.end();
		out.push_back(*it);
		while(it != end)
		{
			PointList path;
			Point current = *(out.end()-1);
			Point last;
			do {
				// doesn't matter how far we are, break out here
				if(it == end) break;

				last = *(it++);
				path.clear();
				Line(current, last, path);
			} while(map.PathIsWalkable(path, abs) && distance(current, last) < range);
			it--;
			out.push_back(*it);
			it++;
		}
	}
	bool Reject(Point const & pt, bool abs)
	{
		return !map.SpaceIsWalkable(pt, abs);
	}
};

#pragma warning ( default: 4512 )

}
}
}