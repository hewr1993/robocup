#include "capture.hpp"
#include <sstream>

using namespace std;

Capture::Capture(){
	_capture.open( CV_CAP_OPENNI );
	_capture.set( CV_CAP_OPENNI_IMAGE_GENERATOR_OUTPUT_MODE, CV_CAP_OPENNI_VGA_30HZ );
}

Capture::Capture(string filename ) {
	_capture.open( filename );
	_capture.set( CV_CAP_OPENNI_IMAGE_GENERATOR_OUTPUT_MODE, CV_CAP_OPENNI_VGA_30HZ );
}

bool Capture::isOpened() {
	return _capture.isOpened() && _capture.grab();
}

bool Capture::grab() {
	_capture.grab();
}

bool Capture::setMode( double mode ) {
	return _capture.set( CV_CAP_OPENNI_IMAGE_GENERATOR_OUTPUT_MODE, mode );
}

string Capture::getInfo() {
	stringstream infostream( stringstream::in | stringstream::out);
    infostream << "\nDepth generator output mode:" << endl <<
            "FRAME_WIDTH      " << _capture.get( CV_CAP_PROP_FRAME_WIDTH ) << endl <<
            "FRAME_HEIGHT     " << _capture.get( CV_CAP_PROP_FRAME_HEIGHT ) << endl <<
            "FRAME_MAX_DEPTH  " << _capture.get( CV_CAP_PROP_OPENNI_FRAME_MAX_DEPTH ) << " mm" << endl <<
            "FPS              " << _capture.get( CV_CAP_PROP_FPS ) << endl <<
            "REGISTRATION     " << _capture.get( CV_CAP_PROP_OPENNI_REGISTRATION ) << endl;
    if( _capture.get( CV_CAP_OPENNI_IMAGE_GENERATOR_PRESENT ) )
    {
        infostream <<
            "\nImage generator output mode:" << endl <<
            "FRAME_WIDTH   " << _capture.get( CV_CAP_OPENNI_IMAGE_GENERATOR+CV_CAP_PROP_FRAME_WIDTH ) << endl <<
            "FRAME_HEIGHT  " << _capture.get( CV_CAP_OPENNI_IMAGE_GENERATOR+CV_CAP_PROP_FRAME_HEIGHT ) << endl <<
            "FPS           " << _capture.get( CV_CAP_OPENNI_IMAGE_GENERATOR+CV_CAP_PROP_FPS ) << endl;
    }
    else
    {
        infostream << "\nDevice doesn't contain image generator." << endl;
    }
	return infostream.str();
}

Mat Capture::getDepth() {
	Mat ret;
	_capture.retrieve( ret, CV_CAP_OPENNI_DEPTH_MAP);
	return ret;
}


Mat Capture::getDisparity() {
	Mat ret;
	_capture.retrieve( ret, CV_CAP_OPENNI_DISPARITY_MAP);
	return ret;
}

static void colorizeDisparity( const Mat& gray, Mat& rgb, double maxDisp=-1.f, float S=1.f, float V=1.f )
{
    CV_Assert( !gray.empty() );
    CV_Assert( gray.type() == CV_8UC1 );

    if( maxDisp <= 0 )
    {
        maxDisp = 0;
        minMaxLoc( gray, 0, &maxDisp );
    }

    rgb.create( gray.size(), CV_8UC3 );
    rgb = Scalar::all(0);
    if( maxDisp < 1 )
        return;

    for( int y = 0; y < gray.rows; y++ )
    {
        for( int x = 0; x < gray.cols; x++ )
        {
            uchar d = gray.at<uchar>(y,x);
            unsigned int H = ((uchar)maxDisp - d) * 240 / (uchar)maxDisp;

            unsigned int hi = (H/60) % 6;
            float f = H/60.f - H/60;
            float p = V * (1 - S);
            float q = V * (1 - f * S);
            float t = V * (1 - (1 - f) * S);

            Point3f res;

            if( hi == 0 ) //R = V,  G = t,  B = p
                res = Point3f( p, t, V );
            if( hi == 1 ) // R = q, G = V,  B = p
                res = Point3f( p, V, q );
            if( hi == 2 ) // R = p, G = V,  B = t
                res = Point3f( t, V, p );
            if( hi == 3 ) // R = p, G = q,  B = V
                res = Point3f( V, q, p );
            if( hi == 4 ) // R = t, G = p,  B = V
                res = Point3f( V, p, t );
            if( hi == 5 ) // R = V, G = p,  B = q
                res = Point3f( q, p, V );

            uchar b = (uchar)(std::max(0.f, std::min (res.x, 1.f)) * 255.f);
            uchar g = (uchar)(std::max(0.f, std::min (res.y, 1.f)) * 255.f);
            uchar r = (uchar)(std::max(0.f, std::min (res.z, 1.f)) * 255.f);

            rgb.at<Point3_<uchar> >(y,x) = Point3_<uchar>(b, g, r);
        }
    }
}

Mat Capture::getColorDisparity() {
	Mat disparityMap = getDisparity();
	Mat colorDisparityMap;
	colorizeDisparity( disparityMap, colorDisparityMap, -1 );
	Mat validColorDisparityMap;
	colorDisparityMap.copyTo( validColorDisparityMap, disparityMap != 0 );
	return validColorDisparityMap;
}

Mat Capture::getValidDepthMask() {
	Mat validDepthMap;
	_capture.retrieve( validDepthMap, CV_CAP_OPENNI_BGR_IMAGE );
	return validDepthMap;
}

Mat Capture::getBGRImage() {
	Mat bgrImage;
	_capture.retrieve( bgrImage , CV_CAP_OPENNI_BGR_IMAGE);
	return bgrImage;
}

Mat Capture::getGrayImage() {
	Mat gray;
	_capture.retrieve( gray, CV_CAP_OPENNI_GRAY_IMAGE);
	return gray;
}
