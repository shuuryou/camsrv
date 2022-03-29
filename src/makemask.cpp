/*
 * makemask - Motion Detection Mask File Creator and Tester
 *
 */

#include "makemask.hpp"

int main(int argc, char* const argv[])
{
	if (argc < 2 || argc > 3)
	{
		printf("\n");
		printf("Motion Detection Mask File Creator and Tester\n");
		printf("\n");
		printf("Usage: %s videofile [maskfile] \n", argv[0]);
		printf("\n");
		printf("videofile    Full path to a video file to use.\n");
		printf("maskfile     Full path to the mask bitmap to use.\n");
		printf("\n");
		printf("You must always specify a video file.\n");
		printf("\n");
		printf("If you do not specify a mask file, the program will create a bitmap file\n");
		printf("called \"mask.bmp\" in the current directory. Use it as a template for\n");
		printf("creating a your own mask for a camera.\n");
		printf("\n");
		printf("In a mask, white pixels are areas to include and black pixels are areas\n");
		printf("to exclude. Use 1bpp (i.e. monochrome) bitmaps to save a lot of space.\n");
		printf("\n");
		printf("If you specify a mask file, the program will create a bitmap file called\n");
		printf("\"result.bmp\" in the current directory. You can use this to check what\n");
		printf("motion detection will 'see' when processing a video file.\n");
		printf("\n");
		exit(-EINVAL);
	}

	string input_file = string(argv[1]);

	if (!filesystem::exists(input_file))
	{
		printf("Error: Input file \"%s\" was not found.\n", input_file.c_str());
		exit(1);
	}

	bool success = true;

	VideoCapture capture;
	Mat frame, mask;

	capture = VideoCapture(input_file);

	if (!capture.isOpened())
	{
		printf("Error: Video file \"%s\" could not be opened.\n", input_file.c_str());
		exit(1);
	}

	if (argc == 3)
	{
		string mask_file = string(argv[2]);

		if (!mask_file.empty() && filesystem::exists(mask_file))
		{
			mask = imread(mask_file, IMREAD_GRAYSCALE);

			if (mask.empty())
			{
				printf("Error: Mask file \"%s\" could not be read.\n", mask_file.c_str());
				success = false;
				goto done;
			}
		}
	}

	printf("Extracting first frame...");
	capture >> frame;
	printf(" OK.\n");

	if (mask.empty())
	{
		imwrite("mask.bmp", frame);
		printf("Success: Mask file template was written to \"mask.bmp\".\n");
		goto done;
	}

	printf("Applying mask to first frame...");
	cvtColor(frame, frame, COLOR_RGB2GRAY);
	try_apply_mask(frame, mask);
	printf(" OK.\n");

	imwrite("result.bmp", frame);
	printf("Success: Result file was written to \"result.bmp\".\n");

done:
	frame.release();
	mask.release();
	capture.release();

	return success ? 0 : 1;
}

inline void try_apply_mask(Mat& matrix, Mat mask)
{
	// Compare with maintenance.cpp

	if (mask.empty()) return;

	Mat tmp;
	matrix.copyTo(tmp, mask);
	matrix.release();
	matrix = tmp;

	/*
	 * Old code, slower since OpenCV can magically parallelize this
	 * by using Intel Thread Building Blocks:
	 *
	 * for(int row = 0; row < matrix.rows; row++)
	 * 	for(int col = 0; col < matrix.cols; col++)
	 * 	{
	 * 		uchar pixel = mask.at<uchar>(row, col);
	 * 		if (pixel == 0) matrix.at<uchar>(row, col) = 0;
	 * 	}
	 */
}
