#include <stdio.h>
#include <opencv2/opencv.hpp>
//#include "opencv2/highgui/highgui.hpp>"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>


using namespace cv;
using namespace std;

int main(int argc, char** argv ) {
	//initialize stream
	cv::VideoCapture stream; 
	if(!stream.open(0)) return 0;
	stream.set(CV_CAP_PROP_BRIGHTNESS, 0.5);
	stream.set(CV_CAP_PROP_CONTRAST, 1.0);
	stream.set(CV_CAP_PROP_SATURATION, 1.0);
	stream.set(CV_CAP_PROP_EXPOSURE, 0);

	while(1) {
		Mat frame, fbw;

		//Capture image, grayscale, then blur
		stream >> frame;
		cv::cvtColor(frame, fbw, COLOR_BGR2GRAY);
		cv::blur(fbw, fbw, Size(3,3));  

		//Canny then contour
		Canny(fbw, fbw, 100, 100*2, 3);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(fbw, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0,0) );
		vector<vector<Point> > hull( contours.size() ); 
		for(int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], hull[i]);
		}
		
		//Calculate moments, centroids of each hull, angle of each hull, then final targets	
		vector<Mat> hullImage(hull.size());
		vector<Moments> hullMoments(hull.size());
		vector<Point> centroids(hull.size());
		vector<Point> targets;
		for(int i = 0; i < hull.size(); i++) {
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			drawContours(colormat, hull, i, Scalar(255,255,255), CV_FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);
			hullMoments[i] = moments(hullImage[i], true);
			centroids[i] = Point(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00); 
			//printf("m00: %f m10 %f m01 %f \n", hullMoments[i].m00, hullMoments[i].m10, 3.0);
		}
		
		
		//Draw everything...
		Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		for( int i = 0; i< contours.size(); i++ ) {
       			Scalar white = Scalar(255, 255, 255 );	//gbr color space...
			Scalar red = Scalar(0, 0, 255);
			Scalar cyan = Scalar(255, 255, 0);
       			drawContours( drawing, contours, i, white, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, red, CV_FILLED);
			circle(drawing, centroids[i], 3, cyan);
		}

		//display
		cv::imshow("frame", drawing);
		cv::imshow("original", frame);
		if(cv::waitKey(10)==27)
		{
			stream.release();
			break;
		}
		//break;
	}
	return 0;
}
