#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>
#include <math.h>


#define COLOR_WHITE Scalar(255, 255, 255 )	//gbr color space...
#define COLOR_RED Scalar(0, 0, 255)
#define COLOR_CYAN Scalar(255, 255, 0)
#define COLOR_ORANGE Scalar(0, 128, 255)

using namespace cv;
using namespace std;

float magnitude(Point2d p) {
	return sqrt(p.x * p.x + p.y * p.y);

}

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
		vector<Point2d> centroids(hull.size());
		vector<double> angles(hull.size());
		vector<Point2d> targets;
		//temp
		vector<double> mp11s;
		vector<double> mp20s;
		vector<double> mp02s;
		//temp		
		vector<Point2d> o_target(hull.size());

		for(int i = 0; i < hull.size(); i++) {
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			drawContours(colormat, hull, i, Scalar(255,255,255), FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);
			
			//for(int z = 0; z < hullImage[i].

			hullMoments[i] = moments(hullImage[i], true);
			centroids[i] = Point2d(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00); 
			double mp11 = hullMoments[i].m11/hullMoments[i].m00 - centroids[i].x * centroids[i].y;
			double mp20 = hullMoments[i].m20/hullMoments[i].m00 - centroids[i].x * centroids[i].x;
			double mp02 = hullMoments[i].m02/hullMoments[i].m00 - centroids[i].y * centroids[i].y;
			
			//temp
			mp11 = hullMoments[i].mu11/hullMoments[i].m00;
			mp20 = hullMoments[i].mu20/hullMoments[i].m00;
			mp02 = hullMoments[i].mu02/hullMoments[i].m00;
			//temp
			mp11s.push_back(mp11);
			mp20s.push_back(mp20);
			mp02s.push_back(mp02);
			//temp


			//angles[i] = 0.5 * atan( (2.0 * mp11)/(mp20-mp02) ); 
			double y = 2.0*mp11;
			double x = mp20-mp02;
			
			angles[i] = 0.5 * atan2( y, x );
			if(y<0) angles[i] += M_PI;

			o_target[i] = Point2d(cos(angles[i]), sin(angles[i]));
			printf("theta: %f cos %f sin %f\n", angles[i], cos(angles[i]), sin(angles[i])); 
			//printf("num: %f denom: %f theta: %f\n", (2.0 * mp11), (mp20-mp02), angles[i]);

		}
		
		//find the target pairs
		vector<vector<Point2d> > projections;
		vector<Point> pairs;
		
		for(int i = 0; i < hull.size(); i++) {
			vector<Point2d> current;
			for(int n = i+1; n < hull.size(); n++) {
				//printf("%f %f\n", o_target[i].x, o_target[i].y); 

				Point2d connector = centroids[n] - centroids[i];

				//printf("%f %f\n", connector.x, connector.y); 
				Point2d proj_i = (connector.ddot(o_target[i])/(connector.x*connector.x + connector.y*connector.y)) * connector;//proj of i on connector
				Point2d proj_n = (connector.ddot(o_target[n])/(connector.x*connector.x + connector.y*connector.y)) * connector;//proj of i on connector
				current.push_back(proj_i);
				current.push_back(proj_n);
				//if(abs(dot_pi-dot_pn) <= 1.0 && dot_pi <= 0 && dot_pn >= 0) printf("pair is %d and %d\n", i, n);
				//printf("proji: x %f y %f ---- projn x %f y %f \n",proj_i.x, proj_i.y, proj_n.x, proj_n.y);
				Point2d sum = proj_i + proj_n;
				double mag = magnitude(sum);
				if(mag <= 0.3 && connector.ddot(o_target[i]) < 0) pairs.push_back(Point(i, n)); 
			}
			projections.push_back(current);	
		}
		printf("\n");
		//for(int i = 0; i < pairs.size(); i++) printf("%d %d \n\n", pairs[i].x, pairs[i].y);
		
		//Draw everything...
		Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		for( int i = 0; i< contours.size(); i++ ) {
       			drawContours( drawing, contours, i, COLOR_WHITE, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, COLOR_RED, FILLED);
			circle(drawing, centroids[i], 3, COLOR_CYAN, -1);
			line(drawing, centroids[i], Point( centroids[i].x+20*cos(angles[i]), centroids[i].y+20*sin(angles[i])), COLOR_CYAN,2);
			
			/*
			for(int n = i+1; n < contours.size(); n++) {
				line(drawing, centroids[i], centroids[n], orange, 2);
					
			}*/	
			

			char c[2];
			sprintf(c, "%d", i);
			putText(drawing, c, centroids[i], FONT_HERSHEY_SIMPLEX, 1, COLOR_WHITE, 2, LINE_AA); 

			//draw projections
			//for(int n = 0; n < projections[i].size(); n++) {
			//	line(drawing, centroids[n], centroids[n] + projections[i][n]*170, COLOR_WHITE, 3);
			//}		

		}
		//draw pair connectors and centroids only if they are an actual pair
		for(int n = 0; n <pairs.size(); n++) {
			line(drawing, centroids[pairs[n].x], centroids[pairs[n].y], COLOR_ORANGE, 2);
			circle(drawing, (centroids[pairs[n].x] + centroids[pairs[n].y])/2, 5, COLOR_ORANGE, -1);
		}
		
		//display
		cv::imshow("frame", drawing);
		//cv::imshow("original", frame);
		if(cv::waitKey(10)==27)
		{
			stream.release();
			break;
		}
		//break;
	}
	return 0;
}
