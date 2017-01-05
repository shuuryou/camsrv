/*
 * mdetect - CamSrv's Motion Detector
 *
 * Shoplifted from <https://github.com/jooray/motion-detection>, who originally
 * shoplifted it from <https://blog.cedric.ws/opencv-simple-motion-detection>.
 *
 * Heavily extended and cleaned up to use OpenCV's C++ API exclusively, because
 * the originals used a big mix of the C and C++ APIs; most likely because the
 * above authors simply just pasted together random example code as well.
 * 
 */

#include "mdetect.hpp"

int main (int argc, char* const argv[])
{
	char* input_file	= NULL;
	char* mask_file		= NULL;
	
	int sensitivity		= SENSITIVITY_DEFAULT;
	int continuation	= CONTINUATION_DEFAULT;
	int maxdeviation	= MAXDEVIATION_DEFAULT;
	
	int last_motion_at	= -1;
	
	bool verbose 		= false;
	bool extract_only	= false;
	
	{
		int opt;
		
		while ((opt = getopt(argc, argv, "f:m:t:d:c:veh")) != EOF)	
			switch(opt)
			{
				case 'f': 
					input_file = optarg;
					break;
				case 'm': 
					mask_file = optarg;
					break;
				case 'c': 
				case 'd': 
				case 't':
					if (!is_number(optarg))
					{
						cerr << "Value \"" << optarg << "\" is not a number." << endl;
						exit_usage();
					}
					
					switch (opt)
					{
						case 't': sensitivity = atoi(optarg); break;
						case 'd': maxdeviation = atoi(optarg); break;
						case 'c': continuation = atoi(optarg); break;
					}
					break;
				case 'v':
					verbose = true;
					break;
				case 'e':
					extract_only = true;
					break;
				case 'h':
				case '?':
				default:
					exit_usage();
			}
	}
	
	if (input_file == NULL)
	{
		cerr << "Missing input file." << endl;
		exit_usage();
	}
	else if (!file_exists(input_file))
	{
		cerr << "Input file not found." << endl;
		exit_usage();
	}

	VideoCapture capture = VideoCapture(input_file);

	if (!capture.isOpened())
	{
		cerr << "Video file could not be read." << endl;
		exit_usage();
	}
	
	Mat mask;
	
	if (mask_file != NULL)
	{
		if (!file_exists(mask_file))
		{
			cerr << "Mask file not found." << endl;
			exit_usage();
		}

		mask = imread(mask_file, CV_LOAD_IMAGE_GRAYSCALE);
		mask = mask > 128; // Force mask to 1bpp/black and white
	}
	
	Mat prev_frame, current_frame, next_frame;
	
	capture >> prev_frame;
	
	if (mask_file == NULL) // If there was no mask, we need to make one
		mask = Mat(prev_frame.rows, prev_frame.cols, CV_8UC1, Scalar(255)); 
	
	cvtColor(prev_frame, prev_frame, CV_RGB2GRAY);
	apply_mask(prev_frame, mask);
	
	if (extract_only)
	{
		imwrite("output.bmp", prev_frame);
		cout << "Done. The file \"output.bmp\" contains the result." << endl;
		return 0;
	}

	capture >> current_frame;
	cvtColor(current_frame, current_frame, CV_RGB2GRAY);
	apply_mask(current_frame, mask);
	
	capture >> next_frame;
	cvtColor(next_frame, next_frame, CV_RGB2GRAY);
	apply_mask(next_frame, mask);

	// d1 and d2 for calculating the differences
	// number_of_changes, the amount of changes in the result matrix.
	Mat d1, d2;
	Mat motion;

	int number_of_changes;
	int number_of_sequence = 0;

	bool motion_found = false;

	Mat erode_kernel = getStructuringElement(MORPH_RECT, Size(2,2));

	while (capture.grab())
	{
		prev_frame = current_frame;
		current_frame = next_frame;

		capture.retrieve(next_frame);
		cvtColor(next_frame, next_frame, CV_RGB2GRAY);
		apply_mask(next_frame, mask);
	
		// Calculate the difference between the images and then do a bitwise AND.
		// Apply threshold and erode so that low differences, e.g. contrast change
		// due to sunlight or falling rain, are ignored.

		absdiff(prev_frame, next_frame, d1);
		absdiff(next_frame, current_frame, d2);
		bitwise_and(d1, d2, motion);
		
		threshold(motion, motion, 35, 255, CV_THRESH_BINARY);
		erode(motion, motion, erode_kernel);

		number_of_changes = detect_motion(motion, maxdeviation);
		
		if (verbose) cout << number_of_changes << "," << flush;

		// If there are enough changes over a large enough number of frames,
		// consider it to be motion and output time.
		if(number_of_changes >= sensitivity)
		{
			if(number_of_sequence >= continuation)
			{ 
				int pos = capture.get(CV_CAP_PROP_POS_MSEC) / 1000;
				if (pos > last_motion_at)
				{
					cout << pos << endl;
					last_motion_at = pos;
				}
			}
			
			number_of_sequence++;
		}
		else
		{
			number_of_sequence = 0;
		}
	}
	
	capture.release();
	prev_frame.release();
	current_frame.release();
	next_frame.release();
		
	return motion_found ? 1 : 0;
}

inline void apply_mask(Mat matrix, Mat mask)
{
	for(int row = 0; row < matrix.rows; row++)
		for(int col = 0; col < matrix.cols; col++)
		{
			uchar pixel = mask.at<uchar>(row, col);
			if (pixel == 0) matrix.at<uchar>(row, col) = 0;
		}
}

inline int detect_motion(const Mat& frame, int maxdeviation)
{
	// Check if there is motion, count the number of changes and return.
	
	// calculate the standard deviation
	Scalar mean, stddev;
	meanStdDev(frame, mean, stddev);

	// If the activity is not spread all throughout the image, then it
	// must be genuine motion and not something like sun glare, branches
	// moving in the wind, or heavy snowfall.
	if(stddev[0] < maxdeviation)
	{
		int number_of_changes = 0;

		// Now loop over the image and find changes
		for(int i = 0; i < frame.rows; i+=2) // height
		{
			for(int j = 0; j < frame.cols; j+=2) // width
			{
				// If the pixel at (i,j) is white (255), it is different
				// in the sequence prev_frame, current_frame, next_frame
				if(static_cast<int>(frame.at<uchar>(i,j)) == 255)
					number_of_changes++;
			}
		}

		return number_of_changes;
	}
	
	return 0;
}

bool file_exists(const string& name)
{
	// One of the ways from <http://stackoverflow.com/a/12774387>
	ifstream f(name.c_str());
	return f.good();
}

bool is_number(const string& s)
{
	// One of the ways from <http://stackoverflow.com/a/4654718>
	string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

void exit_usage()
{
	cerr << endl;
	cerr << "mdetect - CamSrv's Motion Detector" << endl;
	cerr << endl;
	cerr << "Looks for motion in a video file. If it finds some, outputs the offset in" << endl;
	cerr << "seconds where the motion occurred to standard output." << endl;
	cerr << endl;
	cerr << "Returns 0 if no motion is found, otherwise returns 1.";
	cerr << endl;
	cerr << "Usage: mdetect -f file [-m maskfile] [-t sensitivity] [-d maxdeviation] [-c continuation] [-v] [-e] [-h]" << endl;
	cerr << endl;
	cerr << "    -f file         The video file to analyze. This is required." << endl;
	cerr << endl;
	cerr << "    -m maskfile     A black and white bitmap used to tell the program any" << endl;
	cerr << "                    areas it should skip. Any black areas will end up not" << endl;
	cerr << "                    getting analyzed." << endl;
	cerr << endl;
	cerr << "    -t sensitivity  Higher values will detect less motion. Enjoy fiddling" << endl;
	cerr << "                    with this until you get it right." << endl;
	cerr << endl;
	cerr << "    -d maxdeviation Controls how concentrated to a single area any motion" << endl;
	cerr << "                    has to be. Lower values mean the motion has to be in" << endl;
	cerr << "                    a very constrained area. Higher values allow for more" << endl;
	cerr << "                    spaced out motion throughout the scene, which could" << endl;
	cerr << "                    lead to more false positives (e.g. during snowfall or" << endl;
	cerr << "                    when rays of the sun hit into the camera lens.)" << endl;
	cerr << endl;
	cerr << "    -c continuation Controls how many frames any movement has to continue" << endl;
	cerr << "                    before it is considered to be motion." << endl;
	cerr << endl;
	cerr << "    -v              Turns on debugging output." << endl;
	cerr << endl;
	cerr << "    -e              Extracts the first frame from the video file, applies" << endl;
	cerr << "                    the mask (if any) and then saves it as \"output.bmp\"" << endl;
	cerr << "                    to the current directory. Useful to test if your mask" << endl;
	cerr << "                    is working correctly or to get the dimensions to use." << endl;
	cerr << endl;
	cerr << "    -h              Prints this message and exits with EINVAL." << endl;
	cerr << endl;
	cerr << "Compiled default value for -t is " << SENSITIVITY_DEFAULT << "." << endl;
	cerr << "Compiled default value for -d is " << MAXDEVIATION_DEFAULT << "." << endl;
	cerr << "Compiled default value for -c is " << CONTINUATION_DEFAULT << "." << endl;
	cerr << endl;
	exit(-EINVAL);
}
