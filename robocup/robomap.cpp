#include "robomap.hpp"
#include <cstdio>
#include <vector>

using namespace std;
using namespace cv;

string RoboMap::strFilename;
Mat RoboMap::matMap;
int RoboMap::target_col;
int RoboMap::target_row;

int RoboMap::loadMap(string file)
{
	strFilename = file;
	FILE *fp = fopen(file.c_str(),"r");
	if(fp == 0)
		return 1;
	char c;
	uchar uc = 0;
	int flag, col = 0, row = 0, cols = 0, rows = 0;
	vector<uchar> vec;
	for(flag = fscanf(fp, "%c", &c); flag != EOF && flag > 0; flag = fscanf(fp, "%c", &c))
	{
		if(c != '\n')
		{
			switch(c)
			{
				case '*':
					target_col = col;
					target_row = row;
					// no break
				case '0':
					uc = 0;
					break;
				default:
					uc = c;
			}
			vec.push_back(uc);
			++row;
		}
		else
		{
			if(row == 0)
				break;
			if(rows == 0)
				rows = row;
			if(rows != row)
				return 2;
			++col;
			row = 0;
		}
	}
	if(row != 0 && row != rows)
		return 2;
	cols = col;
	matMap.create(cols, rows, CV_8UC1);
	vector<uchar>::iterator iter = vec.begin();
	uchar* ptr = (uchar*)matMap.data;
	for(; iter != vec.end(); ++iter, ++ptr)
		*ptr = *iter;
	fclose(fp);
	return 0;
}

Mat RoboMap::getMap()
{
	return matMap;
}

