/*
 * makemask - Motion Detection Mask File Creator
 *
 */

#ifndef MAKEMASK_HPP
#define MAKEMASK_HPP

#include <stdio.h>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace boost;
using namespace cv;

int main (int argc, char* const argv[]);
void try_apply_mask(Mat& matrix, Mat mask);
#endif