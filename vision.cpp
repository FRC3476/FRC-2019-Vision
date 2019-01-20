#include <stdio.h>
#include <opencv2/opencv.hpp>
//#include "opencv2/highgui/highgui.hpp>"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>


using namespace cv;
using namespace std;

int main(int argc, char** argv ) {
	cv::VideoCapture stream; 
	//Mat frame, fbw;
	
	if(!stream.open(0)) return 0;

	stream.set(CV_CAP_PROP_BRIGHTNESS, 0.5);
	stream.set(CV_CAP_PROP_CONTRAST, 1.0);
	stream.set(CV_CAP_PROP_SATURATION, 1.0);
	//stream.set(CV_CAP_PROP_AUTO_EXPOSURE, 0.25);
	stream.set(CV_CAP_PROP_EXPOSURE, 0);

	while(1) {

		Mat frame, fbw;
		//Capture image, grayscale, then blur
		stream >> frame;
		//frame = imread(argv[1]);
		cv::cvtColor(frame, fbw, COLOR_BGR2GRAY);
		cv::blur(fbw, fbw, Size(3,3));  

		//canny	
		Canny(fbw, fbw, 100, 100*2, 3);
		vector<vector<Point> > contours;
		//vector<vector<Point> > hull( contours.size() );
		vector<Vec4i> hierarchy;
		findContours(fbw, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0,0) );
		vector<vector<Point> > hull( contours.size() );
		vector<Point> points; 
		//vector<Moments> m;
		//moments(hull, true);
		for(int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], hull[i]);
			points.push_back(Point(0, 0));
			for(int n = 0; n < hull[i].size(); n++) {
				points[i] += hull[i][n];
			} 
			points[i] = points[i]/(int)(hull[i].size());
			//printf("%d", 
		}
		
	
		vector<Mat> hullImage(hull.size());
		vector<Moments> hullMoments(hull.size());
		vector<Point> centroids(hull.size());
		for(int i = 0; i < hull.size(); i++) {
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			//Mat graymat = Mat::zeros( fbw.size(), CV_8UC1);
			drawContours(colormat, hull, i, Scalar(255,255,255), CV_FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);
			//threshold(graymat, hullImage[i], 100,255,THRESH_BINARY );
			hullMoments[i] = moments(hullImage[i], true);
			centroids[i] = Point(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00); 
			//printf("m00: %f m10 %f m01 %f \n", hullMoments[i].m00, hullMoments[i].m10, 3.0);
		}
		//if(hull.size()>0)imshow("frame2", hullImage[0]);
		

		Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		for( int i = 0; i< contours.size(); i++ ) {
       			Scalar color = Scalar( /*rng.uniform(0, 255)*/ 255, /*rng.uniform(0,255)*/ 255, /*rng.uniform(0,255)*/ 255 );
			Scalar color2 = Scalar(0, 0, 255);
			Scalar color3 = Scalar(255, 255, 0);
       			drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, color2, CV_FILLED);
			circle(drawing, centroids[i] /*points[i]*/, 3, color3);
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
