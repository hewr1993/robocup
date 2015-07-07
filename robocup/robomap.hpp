#ifndef __ROBOMAP_HPP
#define __ROBOMAP_HPP

#include <string>
#include <opencv2/highgui/highgui.hpp>

class RoboMap{
	static std::string strFilename;
	static cv::Mat matMap;

	public:
		static int target_col, target_row;

		virtual ~RoboMap() = 0;
		static int loadMap(std::string);
		static cv::Mat getMap();
};

#endif
