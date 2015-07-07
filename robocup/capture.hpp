#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

using namespace std;
using namespace cv;

class Capture {
public:
	Capture();
	Capture(string);
	bool isOpened();
	bool grab();
	bool setMode( double );
	string getInfo();
	Mat getDepth();
	Mat getColorDisparity();
	Mat getDisparity();
	Mat getValidDepthMask();
	Mat getBGRImage();
	Mat getGrayImage();
private:
	VideoCapture _capture;
};
