#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "network.h"

#define COLOR_WHITE Scalar(255, 255, 255)	//bgr color space...
#define COLOR_RED Scalar(0, 0, 255)
#define COLOR_CYAN Scalar(255, 255, 0)
#define COLOR_ORANGE Scalar(0, 128, 255)

#define MAX_MAG_ERROR 0.0025 
//0.0015
#define SIZE_TO_DISTANCE_RATIO 2.15
#define MAX_SIZE_TO_DISTANCE_ERROR 1.5 
//1.5

using namespace cv;
using namespace std;
/*
struct exp_data {
	Point2d centroid;
	float connectorMag;

}; */


//Return the magnitude of a vector
float magnitude(Point2d p) {
	return sqrt(p.x * p.x + p.y * p.y);

}


int main(int argc, char** argv ) {
	//initialize stream and camera parameters
	std::string dataline;
	cv::VideoCapture stream; 
	cv::VideoWriter writer; 
	//writer.open("appsrc ! autovideoconvert ! omxh264enc control-rate=2 bitrate=4000000 ! 'video/x-h264, stream-format=(string)byte-stream' ! h264parse ! rtph264pay mtu=1400 ! udpsink host=127.0.0.1 clients=10.10.40.86:5000 port=5000 sync=false async=false ", 0, (double) 5, cv::Size(640,480), true);
	//writer.open("appsrc ! autovideoconvert ! video/x-raw, width=640, height=480 ! omxh264enc control-rate=2 bitrate=4000000 ! video/x-h264, stream-format=byte-stream ! h264parse ! rtph264pay mtu=1400 ! udpsink host=127.0.0.1 clients=10.10.40.86:5000 port=5000 sync=false async=false ", 0, (double) 5, cv::Size(640, 480), true);
	if(!stream.open(2)) return 0;
	//These settings might not work
	//We might have to set these in the startup script
	stream.set(CAP_PROP_BRIGHTNESS, 0.5);
	stream.set(CAP_PROP_CONTRAST, 1.0);
	stream.set(CAP_PROP_SATURATION, 1.0);
	stream.set(CAP_PROP_EXPOSURE, 0);

	setupUDP();
	initLog();
	while(1) {
		std::stringstream logLine;
		std::vector<exp_data> data; 
		
		//Capture image, grayscale, then blur
		Mat frame, fbw;
		//Reading the img takes a pretty long time
		//So we can try putting this on a separate thread
		stream >> frame;
		//frame = imread(argv[1]);
		cv::cvtColor(frame, fbw, COLOR_BGR2GRAY);
		cv::blur(fbw, fbw, Size(3,3));  
		
		/*You dont need to do canny. The exposure
		should be low enough so that the only
		thing you see are the glowing tape. So
		you should be able to just do findContours*/

		//Canny edge detect then contour detect
		Canny(fbw, fbw, 100, 100*2, 3);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(fbw, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0,0) );
		
		
		//Convex hull on contours
		vector<vector<Point> > hull( contours.size() ); 
		for(int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], hull[i]);
		}
		
		
		vector<Mat> hullImage(hull.size()); //hull points mapped to a drawing in order to get moments
		vector<Moments> hullMoments(hull.size()); //1st, 2nd and 3rd order moments of each hull
		vector<Point2d> centroids(hull.size());	//centroids of each hull
		vector<double> angles(hull.size()); //angle from x axis of each hull
		vector<Point2d> targets; //centroids of each FRC vision target
		vector<Point2d> o_target(hull.size()); //normalized direction vector of each hull

		for(int i = 0; i < hull.size(); i++) {
			//Grayscale hull drawings to calculate moments
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			drawContours(colormat, hull, i, Scalar(255,255,255), FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);

			//calculate moments and centroids
			hullMoments[i] = moments(hullImage[i], true);
			centroids[i] = Point2d(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00); 
			
			//calculate second order mu prime central moments
			double mp11 = hullMoments[i].mu11/hullMoments[i].m00;
			double mp20 = hullMoments[i].mu20/hullMoments[i].m00;
			double mp02 = hullMoments[i].mu02/hullMoments[i].m00;

			double y = 2.0*mp11;
			double x = mp20-mp02;
			
			angles[i] = 0.5 * atan2( y, x );
			if(y<0) angles[i] += M_PI; //keep angles in positive domain

			o_target[i] = Point2d(cos(angles[i]), sin(angles[i]));
		}
		
		//find the target pairs
		vector<vector<Point2d> > projections; //projection vectors
		vector<Point> pairs; //indicies of each pair
		for(int i = 0; i < hull.size(); i++) {
			vector<Point2d> current;
			for(int n = i+1; n < hull.size(); n++) {
				Point2d connector = centroids[n] - centroids[i]; //calculate line that passes two hulls
				
				//projections of hull vectors i and n on connector vector
				Point2d proj_i = (connector.ddot(o_target[i])/
						 (connector.x*connector.x + connector.y*connector.y)) * connector; 
				Point2d proj_n = (connector.ddot(o_target[n])/
						 (connector.x*connector.x + connector.y*connector.y)) * connector; 
				proj_i = proj_i/magnitude(connector);
				proj_n = proj_n/magnitude(connector);
				current.push_back(proj_i);
				current.push_back(proj_n); //TODO normalize projections
				
				//Verify & store pairs by checking that projections net zero and face the right way
				Point2d sum = proj_i + proj_n;
				double mag = magnitude(sum);
				//printf("mag of sum %f\n", mag);
				double ratio = magnitude(connector)/sqrt(hullMoments[i].m00 + hullMoments[n].m00);
				//printf("ratio %f\n", ratio);
				//TODO choose shortest connector pair test
				if(mag <= MAX_MAG_ERROR && connector.ddot(o_target[i]) < 0 
				   && abs(ratio-SIZE_TO_DISTANCE_RATIO) <= MAX_SIZE_TO_DISTANCE_ERROR) {
					
					bool flag = false;
					for(int z = 0; z < pairs.size(); z++) {
						if(pairs[z].x == i || pairs[z].y == i) {
							flag = true;
							if(magnitude(connector) <= magnitude(centroids[pairs[z].x] - centroids[pairs[z].y])) {
								pairs[z] = Point(i, n);
								targets[z] = (centroids[i] + centroids[n])/2;
							} else {
								break;
							} 
						}
					} 
					if(flag == false) {
						pairs.push_back(Point(i, n)); 
						targets.push_back((centroids[i] + centroids[n])/2);
					}
				}
			}
			projections.push_back(current);	
		}
		
		//Draw everything...
		Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		for( int i = 0; i< contours.size(); i++ ) {
       			drawContours( drawing, contours, i, COLOR_WHITE, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, COLOR_RED, FILLED);
			circle(drawing, centroids[i], 3, COLOR_CYAN, -1);
			line(drawing, centroids[i], Point(centroids[i].x+20*cos(angles[i]), centroids[i].y+20*sin(angles[i])), COLOR_CYAN,2);
			
			char c[2];
			sprintf(c, "%d", i);
			putText(drawing, c, centroids[i], FONT_HERSHEY_SIMPLEX, 1, COLOR_WHITE, 2, LINE_AA); 

			//draw projection vectors
			/*
			   for(int n = 0; n < projections[i].size(); n++) {
				line(drawing, centroids[n], centroids[n] + projections[i][n]*170, COLOR_WHITE, 3);
			   }		
			*/
		}
		line(drawing, Point(320, 0), Point(320, 480), COLOR_WHITE, 2);
		//draw pair connectors and centroids only if they are an actual pair
		for(int n = 0; n <pairs.size(); n++) {
			line(drawing, centroids[pairs[n].x], centroids[pairs[n].y], COLOR_ORANGE, 2);
			circle(drawing, (centroids[pairs[n].x] + centroids[pairs[n].y])/2, 5, COLOR_ORANGE, -1);
		}

		for(int n = 0; n < pairs.size(); n++) {
			float mag = magnitude(centroids[pairs[n].x] - centroids[pairs[n].y]);
			exp_data t  = {(centroids[pairs[n].x] + centroids[pairs[n].y])/2, mag}; 
			logLine << centroids[pairs[n].x].x << " " << centroids[pairs[n].x].y << " " 
				<< centroids[pairs[n].y].x << " " << centroids[pairs[n].y].y << " "
				<< hullMoments[pairs[n].x].m00 << " " << hullMoments[pairs[n].y].m00 << "\n";
			log(logLine.str());
			data.push_back(t);
		}
		//Display program vision and original camera frame
		cv::imshow("frame", drawing);
		//cv::imshow("original", frame);
		//waitKey pauses for whatever ms so only put 1 inside
		//also imshow doesnt work if you don't call waitKey
		if(cv::waitKey(10)==27)
		{
			stream.release();
			break;
		}
		sendUDP(data);
		//writer.write(drawing);
	}
	return 0;
}
