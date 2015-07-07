#include "astar.hpp"
#include <cmath>
#include <set>

using namespace std;
using namespace cv;
using namespace astar;

/////////////// private ///////////////

struct ASNode;
typedef ASNode* pASNode;

struct ASNode
{
	int col;
	int row;
	bool pathable;
	pASNode parent;
	set<pASNode> link;
	bool active;
	bool lock;
	int G, H;
};

int as_H(pASNode loc)
{
	// Manhattan Alg
	return (abs(loc->col - RoboMap::target_col) + abs(loc->row - RoboMap::target_row)) * 10;
}

/////////////// public ///////////////

vector<ASPoint> astar::asFindPath(ASPoint loc)
{
	// init
	int cols = RoboMap::getMap().cols, rows = RoboMap::getMap().rows;
	int tcol = RoboMap::target_col, trow = RoboMap::target_row;
	vector< vector<pASNode> > nodes;
	nodes.resize(cols);
	for(int col = 0; col < cols; ++col)
	{
		nodes[col].resize(rows);
		for(int row = 0; row < rows; ++row)
		{
			nodes[col][row] = new ASNode;
			nodes[col][row]->col = col;
			nodes[col][row]->row = row;
			nodes[col][row]->pathable = RoboMap::getMap().at<uchar>(col, row) == 0;
			nodes[col][row]->parent = 0;
			nodes[col][row]->active = false;
			nodes[col][row]->lock = false;
			nodes[col][row]->G = -1;
			nodes[col][row]->H = -1;
		}
	}
	for(int col = 0; col < cols; ++col)
		for(int row = 0; row < rows; ++row)
		{
			if(col - 1 >= 0)
				nodes[col - 1][row]->link.insert(nodes[col][row]);
			if(col + 1 < cols)
				nodes[col + 1][row]->link.insert(nodes[col][row]);
			if(row - 1 >= 0)
				nodes[col][row - 1]->link.insert(nodes[col][row]);
			if(row + 1 < rows)
				nodes[col][row + 1]->link.insert(nodes[col][row]);
		}

	// loop
	int col = loc.col, row = loc.row;
	set<pASNode> activeNodes;
	pASNode pn = nodes[col][row];
	pn->G = 0;
	pn->H = as_H(pn);
	while(!nodes[tcol][trow]->active && (activeNodes.size() || pn == nodes[loc.col][loc.row]))
	{
		// lock and update active list
		pn->lock = true;
		activeNodes.erase(pn);
		for(set<pASNode>::iterator iter = pn->link.begin(); iter != pn->link.end(); ++iter)
		{
			if((*iter)->lock || !(*iter)->pathable)
				continue;
			activeNodes.insert(*iter);
			(*iter)->active = true;
			if((*iter)->G == -1 || (*iter)->G > pn->G + 10)
			{
				(*iter)->parent = pn;
				(*iter)->G = pn->G + 10;
			}
			if((*iter)->H == -1)
				(*iter)->H = as_H(*iter);
		}

		// find minimum F in active list
		int minF = -1;
		pn = 0;
		for(set<pASNode>::iterator iter = activeNodes.begin(); iter != activeNodes.end(); ++iter)
		{
			int F = (*iter)->G + (*iter)->H;
			if(minF == -1 || minF > F)
			{
				minF = F;
				pn = *iter;
			}
		}

	}
	if(activeNodes.size() == 0)
	{
		// no path
		vector<ASPoint> vec;
		return vec;
	}

	// rebuild path
	vector<pASNode> rvec;
	for(pn = nodes[tcol][trow]; pn != nodes[col][row]; pn = pn->parent)
		rvec.push_back(pn);
	vector<ASPoint> vec;
	vec.push_back(ASPoint(pn->col, pn->row));
	for(vector<pASNode>::reverse_iterator iter = rvec.rbegin(); iter != rvec.rend(); ++iter)
		vec.push_back(ASPoint((*iter)->col, (*iter)->row));
	return vec;
}

ASDirect astar::asFindDirect(ASPoint loc, ASDirect face)
{
	return asDirectTo(loc, asFindPath(loc)[1], face);
}

ASDirect astar::asDirectTo(ASPoint ptFrom, ASPoint ptTo, ASDirect face)
{
	ASDirect d;
	if(ptTo.row == ptFrom.row + 1 && ptTo.col == ptFrom.col)
		d = RIGHT;
	else if(ptTo.row == ptFrom.row - 1 && ptTo.col == ptFrom.col)
		d = LEFT;
	else if(ptTo.col == ptFrom.col + 1 && ptTo.row == ptFrom.row)
		d = DOWN;
	else if(ptTo.col == ptFrom.col - 1 && ptTo.row == ptFrom.row)
		d = UP;
	else
		return ERROR;
	return ASDirection[(d + 4 - face) % 4];
}

