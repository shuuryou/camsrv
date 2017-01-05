/*
 * mdetect - CamSrv's Motion Detector
 *
 */
 
#ifndef MDETECT_HPP
#define MDETECT_HPP

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <errno.h>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

#define SENSITIVITY_DEFAULT		50
#define CONTINUATION_DEFAULT	2
#define MAXDEVIATION_DEFAULT	20

int main (int argc, char* const argv[]);

void apply_mask(Mat a, Mat mask);
int detect_motion(const Mat& frame, int max_deviation);

bool file_exists(const string& name);
bool is_number(const string& s);

void exit_usage();
#endif