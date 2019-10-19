#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "network.h"
#include <unistd.h>
#include <time.h>
#include <chrono>
#include <numeric>

#define COLOR_WHITE Scalar(255, 255, 255)	//bgr color space...
#define COLOR_RED Scalar(0, 0, 255)
#define COLOR_CYAN Scalar(255, 255, 0)
#define COLOR_ORANGE Scalar(0, 128, 255)

#define MAX_MAG_ERROR 0.0035 //0.0085 
//0.0015
#define SIZE_TO_DISTANCE_RATIO 2.4
#define MAX_SIZE_TO_DISTANCE_ERROR 0.5 
//1.5
#define SMALL_PIXEL_CULL 50

//#define matMoments
#define USE_GSTREAMER 0

#define HIGH_EXP 0.03
#define LOW_EXP 0.001

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
	bool curExpHigh = false;
	//initialize stream and camera parameters
	std::string dataline;
	cv::VideoCapture stream; 
	cv::VideoWriter writer; 
	//stream.set(CAP_PROP_FPS, 60);
	//writer.open("appsrc ! autovideoconvert ! omxh264enc control-rate=2 bitrate=4000000 ! 'video/x-h264, stream-format=(string)byte-stream' ! h264parse ! rtph264pay mtu=1400 ! udpsink host=127.0.0.1 clients=10.10.40.86:5000 port=5000 sync=false async=false ", 0, (double) 5, cv::Size(640,480), true);
	writer.open("appsrc ! autovideoconvert ! video/x-raw, width=640, height=480 ! omxh264enc control-rate=2 bitrate=125000 ! video/x-h264, stream-format=byte-stream ! h264parse ! rtph264pay mtu=1400 ! udpsink host=127.0.0.1 clients=10.34.76.5:5800 port=5800 sync=false async=false ", 0, (double) 5, cv::Size(640, 480), true);
	#if USE_GSTREAMER
		if(!stream.open("v4l2src device=/dev/v4l/by-path/platform-tegra-xhci-usb-0:3.3:1.0-video-index0 ! image/jpeg, width=640, height=480 ! jpegparse ! jpegdec ! videoconvert ! appsink")) return 0;
	#else
		if(!stream.open("/dev/v4l/by-path/platform-tegra-xhci-usb-0:3.3:1.0-video-index0")) return 0;
		stream.set(CAP_PROP_FRAME_WIDTH, 640);
        	stream.set(CAP_PROP_FRAME_HEIGHT,480);
        	stream.set(CAP_PROP_FPS, 60);
	#endif
	//These settings might not work
	//We might have to set these in the startup script
	//stream.set(CAP_PROP_MODE, 0);
	//stream.release();
	//stream.open("/dev/v4l/by-path/platform-tegra-xhci-usb-0:3.4:1.0-video-index0");
	//stream.set(CAP_PROP_MODE, 0);

	stream.set(CAP_PROP_BRIGHTNESS, 0.5);
	stream.set(CAP_PROP_CONTRAST, 1.0);
	stream.set(CAP_PROP_SATURATION, 1.0);
	stream.set(CAP_PROP_EXPOSURE, 0.001); //0.001
	//stream.set(CAP_PROP_FPS, 60);
	//stream.set(CAP_PROP_FRAME_WIDTH, 1920);
	//stream.set(CAP_PROP_FRAME_HEIGHT, 1080);
	//stream.set(CAP_PROP_FPS, 1020);
	//for(int i = 1; i < 2; i++) {
	//	std::cout << stream.set(CAP_PROP_MODE, 0) << std::endl;
	//}
	//stream.set(CAP_PROP_FRAME_WIDTH, 640);
	//stream.set(CAP_PROP_FRAME_HEIGHT,480);
	//stream.set(CAP_PROP_FPS, 60);
	//stream.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
	//stream.set(CAP_PROP_BUFFERSIZE, 3);

	setupUDP();
	printf("setup uDP \n");
	//while(1);
	//initLog();
	auto prevTime = std::chrono::high_resolution_clock::now();
	int c = 0;
	int prevSwitchC = 0;
	usleep(1000000);
	for(int i = 0; i < 30; i++) {
		Mat frame;
		stream >> frame;
	}
	Mat kernel;
	kernel  = cv::getStructuringElement(MORPH_RECT, Size(3,3));
	double fpsA[5] = {0, 0, 0, 0, 0};
	bool trackMode = false;
	while(1) { 
//		printf("bug");
		c+=1;
		if(!stream.isOpened()) return -1;
		std::stringstream logLine;
		std::vector<exp_data> data; 
		
		//Capture image, grayscale, then blur
		Mat frame, colorFilter, fbw;
		//Reading the img takes a pretty long time
		//So we can try putting this on a separate thread
		//std::cout << "reading frame" << std::endl;
		stream >> frame;
		bool expStateHigh = getExposure();
		if(expStateHigh != curExpHigh) {
			curExpHigh = expStateHigh;
			if(expStateHigh) stream.set(CAP_PROP_EXPOSURE, HIGH_EXP);
			else {
				stream.set(CAP_PROP_EXPOSURE, LOW_EXP);
				prevSwitchC = c;
			}
		}
		trackMode = !curExpHigh && c -prevSwitchC>=3;
		auto cur = std::chrono::high_resolution_clock::now();
		//std::chrono::duration<double> delta = cur-prevTime;
		double deltaT = ((double)std::chrono::duration_cast<std::chrono::microseconds>(cur-prevTime).count()/1e6);
		if(deltaT > 2.0) {
			//return -1;

		}
		fpsA[c%5] = 1.0/deltaT;
		double fps = 0;
		for(int i = 0; i < 5; i++) fps+=fpsA[i];
		fps /= 5;
		prevTime = cur;

	//	char fpsStr[5];
           //     sprintf(fpsStr, "%.0f", fps);
          //      putText(frame, fpsStr, Point(610, 10), FONT_HERSHEY_SIMPLEX, 0.5, COLOR_RED, 2, LINE_AA);

		//writer.write(frame);
		//frame = imread(argv[1]);
		//frame = imread("/static-tests/static5.png");
		//cv::blur(frame, frame, Size(10,10));

		//cv::inRange(frame, Scalar(0, 64, 0), Scalar(32, 255, 32), fbw);
		cv::cvtColor(frame, fbw, COLOR_BGR2HSV); 
		cv::inRange(fbw, Scalar(50,200,40), Scalar(70, 255, 255), fbw);
		//cv::cvtColor(frame, frame, COLOR_HSV2BGR);

		//fbw = 255- fbw;
		//std::cout << colorFilter.depth() << std::endl;
		//std::cout << colorFilter.channels() << std::endl;
		cv::morphologyEx(fbw, fbw, MORPH_OPEN, kernel); 

		//cv::cvtColor(fbw, colorFilter, COLOR_GRAY2BGR);
		//writer.write(colorFilter);	
		//continue;
		//cv::cvtColor(frame, fbw, COLOR_BGR2GRAY);
		
		/*You dont need to do canny. The exposure
		should be low enough so that the only
		thing you see are the glowing tape. So
		you should be able to just do findContours*/

		//Canny edge detect then contour detect
		//Canny(fbw, fbw, 100, 100*2, 3);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		
		findContours(fbw, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0,0) );
		
		
		//Convex hull on contours
		vector<vector<Point> > hull( contours.size() ); 
		for(int i = 0; i < contours.size(); i++) {
			convexHull(contours[i], hull[i]);
		}		

		#ifdef matMoments
		vector<Mat> hullImage(hull.size()); //hull points mapped to a drawing in order to get moments
		#endif
		

		vector<Moments> hullMoments(hull.size()); //1st, 2nd and 3rd order moments of each hull
		vector<Point2d> centroids(hull.size());	//centroids of each hull
		vector<double> angles(hull.size()); //angle from x axis of each hull
		vector<Point2d> targets; //centroids of each FRC vision target
		vector<Point2d> o_target(hull.size()); //normalized direction vector of each hull

		auto start = std::chrono::high_resolution_clock::now();
		auto end = std::chrono::high_resolution_clock::now();
		for(int i = 0; i < hull.size(); i++) {
			#ifdef matMoments
			//Grayscale hull drawings to calculate moments
			hullImage[i] = Mat::zeros( fbw.size(), CV_8UC1);
			Mat colormat = Mat::zeros( fbw.size(), CV_8UC3);
			drawContours(colormat, hull, i, Scalar(255,255,255), FILLED);
			cvtColor(colormat, hullImage[i], COLOR_BGR2GRAY);

			//calculate moments and centroids
			start = std::chrono::high_resolution_clock::now();
			//hullMoments[i] = moments(hullImage[i], true);
			#else
			hullMoments[i] = moments(hull[i], true);
			#endif
			//printf("good: %0.2f bad: %0.2f \n", t.m01, hullMoments[i].m01);
			//end = std::chrono::high_resolution_clock::now();
			centroids[i] = Point2d(hullMoments[i].m10/hullMoments[i].m00, hullMoments[i].m01/hullMoments[i].m00);
			//if(hullMoments[i].m00 < 10) ;
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
                //end = std::chrono::high_resolution_clock::now();
		//double dt = ((double)std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()/1e6);
		//printf("segment time: %0.6f  _____ total time: %0.6f \n", dt, deltaT); 
		//find the target pairs
		start = std::chrono::high_resolution_clock::now();

		vector<vector<Point2d> > projections; //projection vectors
		vector<Point> pairs; //indicies of each pair
		for(int i = 0; i < hull.size(); i++) {
			if(hullMoments[i].m00 < SMALL_PIXEL_CULL) continue; //magic numbers
			vector<Point2d> current;
			for(int n = i+1; n < hull.size(); n++) {
				if(hullMoments[n].m00 < SMALL_PIXEL_CULL) continue; //magic numbers
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
								//printf("hull size %n", hull[i].size());
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
		end = std::chrono::high_resolution_clock::now();
		// double dt = ((double)std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()/1e6);
                //printf("segment time: %0.6f  _____ total time: %0.6f \n", dt, deltaT);

		//start = std::chrono::high_resolution_clock::now();

		//Draw everything...
		//Mat drawing = Mat::zeros( fbw.size(), CV_8UC3 );		
  		//cv::cvtColor(fbw, fbw, COLOR_GRAY2BGR);
		cv::cvtColor(fbw, colorFilter, COLOR_GRAY2BGR);




		//Mat drawing = (frame*8)-100;//colorFilter;
		Mat drawing = frame;//frame.clone();
		if(trackMode) {
		for( int i = 0; i< contours.size(); i++ ) {
			//if(hullMoments[i].m00 < SMALL_PIXEL_CULL) continue;
       			//drawContours( drawing, contours, i, COLOR_WHITE, 2, 8, hierarchy, 0, Point() );
      			drawContours(drawing, hull, i, COLOR_RED, 2);
			circle(drawing, centroids[i], 3, COLOR_CYAN, -1);
			line(drawing, centroids[i], Point(centroids[i].x+20*cos(angles[i]), centroids[i].y+20*sin(angles[i])), COLOR_CYAN,2);
			
			char c[2];
			sprintf(c, "%d", i);
			//putText(drawing, c, centroids[i], FONT_HERSHEY_SIMPLEX, 0.3, COLOR_WHITE, 2, LINE_AA); 

			//draw projection vectors
		}
		line(drawing, Point(320, 0), Point(320, 480), COLOR_WHITE, 2);
		//draw pair connectors and centroids only if they are an actual pair
		for(int n = 0; n <pairs.size(); n++) {
			line(drawing, centroids[pairs[n].x], centroids[pairs[n].y], COLOR_ORANGE, 2);
			circle(drawing, (centroids[pairs[n].x] + centroids[pairs[n].y])/2, 5, COLOR_ORANGE, -1);
		}

		for(int n = 0; n < pairs.size(); n++) {
			float mag = magnitude(centroids[pairs[n].x] - centroids[pairs[n].y]);
			float distance = 6232.6 * pow(mag, -1.084);
			exp_data t  = {(centroids[pairs[n].x] + centroids[pairs[n].y])/2, mag, distance}; 
			logLine << centroids[pairs[n].x].x << " " << centroids[pairs[n].x].y << " " 
				<< centroids[pairs[n].y].x << " " << centroids[pairs[n].y].y << " "
				<< hullMoments[pairs[n].x].m00 << " " << hullMoments[pairs[n].y].m00 << "\n";
			char dist[10];
			sprintf(dist, "%.2f", distance);
			putText(drawing, dist, (centroids[pairs[n].x] + centroids[pairs[n].y])/2, FONT_HERSHEY_SIMPLEX, 0.5, COLOR_WHITE, 2, LINE_AA);
			//log(logLine.str());
			data.push_back(t);
		}
		char hullCnt[5];
                sprintf(hullCnt, "%d", (int)hull.size());
                putText(drawing, hullCnt, Point(590, 400), FONT_HERSHEY_SIMPLEX, 0.5, COLOR_RED, 2, LINE_AA);

		}
		if(c%20>10) circle(drawing, Point(10, 10), 3, COLOR_RED, -1);
		char fpsStr[5];
		sprintf(fpsStr, "%.0f", fps);
		putText(drawing, fpsStr, Point(590, 10), FONT_HERSHEY_SIMPLEX, 0.5, COLOR_RED, 2, LINE_AA);

		//Display program vision and original camera frame
		//cv::imshow("frame", drawing);
		//cv::imshow("original", frame);
		//waitKey pauses for whatever ms so only put 1 inside
		//also imshow doesnt work if you don't call waitKey
		
		//end = std::chrono::high_resolution_clock::now();
 		 //double dt = ((double)std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()/1e6);
                //printf("segment time: %0.6f  _____ total time: %0.6f \n", dt, deltaT);
		//cv::waitKey();
		if(trackMode) {
			sendUDP(data);
			//writer.write(drawing);
		} 
		writer.write(drawing);
	}
	return 0;
}
