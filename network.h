#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>

struct exp_data {
	cv::Point2d centroid; 
	float connectorMag;
	float distance;
};

int sendUDP(std::vector<exp_data> d);

void setupUDP();

bool getExposure();
