#include <stdio.h>
#include <opencv2/opencv.hpp>
//#include "opencv2/highgui/highgui.hpp>"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>
#include <math.h>


using namespace cv;
using namespace std;

int main(int argc, char** argv ) {
	//initialize stream
	cv::VideoCapture stream; 
	if(!stream.open(0)) return 0;
	stream.set(CAP_PROP_BRIGHTNESS, 0.5);
	stream.set(CAP_PROP_CONTRAST, 1.0);
	stream.set(CAP_PROP_SATURATION, 1.0);
	stream.set(CAP_PROP_EXPOSURE, 0);

	while(1) {
		Mat frame, fbw;

		//Capture image, grayscale, then blur
		stream >> frame;
		//printf("x:%d y:%d \n", frame.cols, frame.rows); 
		//frame = imread(argv[1]);
		cv::cvtColor(frame, fbw, COLOR_BGR2GRAY);
		cv::blur(fbw, fbw, Size(3,3));  

		//Canny then contour
		Canny(fbw, fbw, 100, 100*2, 3);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(fbw, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0,0) );
		vector<vector<Point> > hull( contours.size() ); 
		for(int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], hull[i]);
		}
		
		//Calculate moments, centroids of each hull, angle of each hull, then final targets	
		vector<Mat> hullImage(hull.size());
		vector<Moments> hullMoments(hull.size());
		vector<Point> centroids(hull.size());
		vector<double> angles(hull.size());
		vector<Point> targets;
		//temp
		vector<double> mp11s;
		vector<double> mp20s;
		vector<double> mp02s;
		//temp		

		for(int i = 0; i < hull.size(); i++) {
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			drawContours(colormat, hull, i, Scalar(255,255,255), FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);
			
			//for(int z = 0; z < hullImage[i].

			hullMoments[i] = moments(hullImage[i], true);
			centroids[i] = Point(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00); 
			double mp11 = hullMoments[i].m11/hullMoments[i].m00 - centroids[i].x * centroids[i].y;
			double mp20 = hullMoments[i].m20/hullMoments[i].m00 - centroids[i].x * centroids[i].x;
			double mp02 = hullMoments[i].m02/hullMoments[i].m00 - centroids[i].y * centroids[i].y;
			
			//temp
			mp11 = hullMoments[i].mu11;
			mp20 = hullMoments[i].mu20;
			mp02 = hullMoments[i].mu02;
			//temp
			mp11s.push_back(mp11);
			mp20s.push_back(mp20);
			mp02s.push_back(mp02);
			//temp

			//angles[i] = 0.5 * atan( (2.0 * mp11)/(mp20-mp02) ); 
			angles[i] = 0.5 * atan2( (2.0 * mp11), (mp20-mp02) );
			//angles[i] = fmod(angles[i], 2 * 3.141592);
			printf("hull %d -mp11: %f\n", i, mp11); 
			//printf("m00 %f mp11: %f m20 %f m02 %f \n", hullMoments[i].m00, mp11, mp20, mp02);
		}
		printf("\n\n");
		
		//Draw everything...
		Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		for( int i = 0; i< contours.size(); i++ ) {
       			Scalar white = Scalar(255, 255, 255 );	//gbr color space...
			Scalar red = Scalar(0, 0, 255);
			Scalar cyan = Scalar(255, 255, 0);
       			drawContours( drawing, contours, i, white, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, red, FILLED);
			circle(drawing, centroids[i], 3, cyan, -1);
			circle(drawing, Point(mp20s[i], mp02s[i]) + centroids[i], 5, Scalar(0, 255, 255), -1);
			line(drawing, centroids[i], Point( centroids[i].x+20*cos(angles[i]), centroids[i].y+20*sin(angles[i])), cyan, 2);
			char c[2];
			sprintf(c, "%d", i);
			putText(drawing, c, centroids[i], FONT_HERSHEY_SIMPLEX, 1, white, 2, LINE_AA); 
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
