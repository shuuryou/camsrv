/*
 * maintenance - Maintenance Program for Camera Recordings
 *
 * Motion detection shoplifted from <https://github.com/jooray/motion-detection>,
 * who shoplifted it from <https://blog.cedric.ws/opencv-simple-motion-detection>.
 *
 * Heavily extended and cleaned up to use OpenCV's C++ API exclusively, because
 * the originals used a big mix of the C and C++ APIs; most likely because the
 * above authors simply just pasted together random example code as well.
 *
 * Nethertheless, thanks to Cédric Verstraeten for the general motion detection
 * algorithm, a mutation of which lives on in this code.
 *
 */

#include "maintenance.hpp"

vector<camera>	m_Cameras;
bool			m_Delete;
bool			m_Motion;
bool			m_Verbose;
bool			m_Syslog;

int main (int argc, char* const argv[])
{
	string configfile;
	m_Syslog = false;
	char opt;

	m_Verbose = false;

	while ((opt = getopt(argc, argv, "c:sv")) != EOF)
		switch(opt)
		{
			case 's':
				m_Syslog = true;
				break;
			case 'c':
				configfile = optarg;
				break;
			case 'v':
				m_Verbose = true;
				break;
			case '?':
			default:
				exit_usage(argv[0]);
				break;
		}

	if (m_Syslog)
	{
		openlog(SYSLOG_IDENT, LOG_CONS, LOG_LOCAL0);
		atexit(closelog);
	}

	int lock_status = check_if_running_and_lock(LOCKFILE);

	switch (lock_status)
	{
		case -2:
			abort(); // Must never happen
		case -1:
			LOG(LOG_ERR, "Could not create lock file.");
			exit(1);
		case  1:
			LOG(LOG_ERR, "Another instance is already running.");
			exit(1);
		case  0:
			// Happy days.
			break;
	}

	if (configfile.empty())
	{
		LOG(LOG_CRIT, "Missing configuration file.");
		exit(1);
	}

	load_settings(configfile);

	LOG(LOG_NOTICE, "Maintenance is starting.");

	if (m_Delete)
		do_delete();
	else if (m_Verbose)
		LOG(LOG_DEBUG, "Delete is turned off.");

	if (m_Motion)
		do_motion();
	else if (m_Verbose)
		LOG(LOG_DEBUG, "Motion detection is turned off.");

	LOG(LOG_NOTICE, "Maintenance has completed.");

	return 0;
}

void exit_usage(const char* argv0)
{
	printf("\n");
	printf("Maintenance Program for Camera Recordings\n");
	printf("\n");
	printf("Usage: %s -c configfile [-s] [-v]\n", argv0);
	printf("\n");
	printf("-c configfile    Full path to camsrv.ini configuration file.\n");
	printf("-s               Send output to syslog instead of stdout.\n");
	printf("-v               Make output a little bit more verbose.\n");
	printf("\n");
	exit(-EINVAL);
}

void do_delete()
{
	LOG(LOG_NOTICE, "Delete is starting.");

	uint processed = 0;
	uintmax_t totalbytes = 0;

	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		struct tm cutoff_tm;
		time_t cutoff = time(NULL);

		localtime_r(&cutoff, &cutoff_tm);
		cutoff_tm.tm_sec = 0;
		cutoff_tm.tm_min = 0;
		cutoff_tm.tm_hour = 0;

		cutoff = mktime(&cutoff_tm);
		cutoff -= (86400 * cam->deleteafterdays);

		assert(cutoff > 0);

		if (m_Verbose)
		{
			localtime_r(&cutoff, &cutoff_tm);

			char buff[30];
			strftime(buff, 30, "%Y-%m-%d %H:%M:%S", &cutoff_tm);

			LOG(LOG_DEBUG, "Cutoff is %s.", buff);
		}

		filesystem::directory_iterator dir(cam->destination), end;

		while (dir != end)
		{
			filesystem::path cur_path = dir->path();

			if (filesystem::is_symlink(dir->path()))
			{
				LOG(LOG_WARNING, "Encountered a symbolic link (\"%s\"). Aborting delete.",
					cur_path.string().c_str());
				break;
			}

			time_t modification_time = filesystem::last_write_time(dir->path());

			if (modification_time < cutoff)
			{
				++processed;
				totalbytes += filesystem::file_size(dir->path());
				filesystem::remove(dir->path());

				LOG(LOG_INFO, "Deleted old video file \"%s\".", cur_path.string().c_str());

				if (m_Verbose)
					LOG(LOG_DEBUG, "modification_time %d, cutoff %d", modification_time, cutoff);
			}

			++dir;
		}
	}

	LOG(LOG_NOTICE, "Delete has completed and deleted %d file(s), freeing %ju byte(s).",
		processed, totalbytes);
}

void do_motion()
{
	LOG(LOG_NOTICE, "Motion detection is starting.");

	const time_t overall_start = time(NULL);

	// To automatically sort input files by mtime without reinventing the wheel
	multimap<time_t, pair<filesystem::path, camera> > files;

	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		filesystem::recursive_directory_iterator dir(cam->destination), end;

		while (dir != end)
		{
			filesystem::path cur_path = dir->path();
			dir++;

			if (filesystem::is_symlink(cur_path))
			{
				LOG(LOG_WARNING, "Encountered a symbolic link (\"%s\").", cur_path.string().c_str());
				abort();
			}

			time_t modification_time = filesystem::last_write_time(cur_path);

			if (time(NULL) - modification_time < 60)
			{
				if (m_Verbose)
				{
					LOG(LOG_DEBUG, "Not processing \"%s\" because it is too young.",
						cur_path.string().c_str());
				}
				continue;
			}

			size_t string_pos = cur_path.string().find("-MOTION");

			if (string_pos != string::npos)
			{
				if (m_Verbose)
				{
					LOG(LOG_DEBUG, "Not processing \"%s\" because it was processed before.",
						cur_path.string().c_str());
				}
				continue;
			}

			if (m_Verbose)
			{
				LOG(LOG_DEBUG, "Queueing \"%s\" for processing.",
					cur_path.string().c_str());
			}

			files.insert(pair<time_t, pair<filesystem::path, camera> >(modification_time,
				pair<filesystem::path, camera>(cur_path, *cam)));
		}
	}

	LOG(LOG_NOTICE, "Motion detection will now process %d video file(s).", files.size());

	for (multimap<time_t, pair<filesystem::path, camera> >::iterator it = files.begin(); it != files.end(); ++it)
	{
		const time_t detection_start = time(NULL);

		const filesystem::path path = (it->second).first;
		const camera cam = (it->second).second;

		if (m_Verbose)
			LOG(LOG_DEBUG, "Processing video file \"%s\".", path.string().c_str());

		int motion_detected = video_motion_detection(path.string(), cam);

		if (motion_detected == -1)
		{
			LOG(LOG_WARNING, "Motion detection failed for video file (\"%s\").",
				path.string().c_str());

			return;
		}

		char* filename_buffer = NULL;

		asprintf(&filename_buffer, "%s-MOTION-%d%s",
			path.stem().string().c_str(),
			motion_detected,
			path.extension().string().c_str());

		filesystem::path new_path =
			path.parent_path() / filesystem::path(filename_buffer);

		free(filename_buffer);

		filesystem::rename(path, new_path);

		const time_t detection_end = time(NULL);

		LOG(LOG_INFO, "Motion detection result for video file \"%s\" was %d. Determined in %d second(s).",
			path.string().c_str(), motion_detected, (detection_end - detection_start));
	}

	files.clear();

	const time_t overall_end = time(NULL);

	LOG(LOG_NOTICE, "Motion detection has completed in %d second(s).",
		(overall_end - overall_start));
}

void load_settings(const string& filename)
{
	if (!filesystem::exists(filename))
	{
		LOG(LOG_CRIT, "Configuration file \"%s\" not found.\n", filename.c_str());
		exit(1);
	}

	property_tree::ptree pt;

	try
	{
		property_tree::ini_parser::read_ini(filename, pt);
	}
	catch (const property_tree::ini_parser::ini_parser_error &e)
	{
		LOG(LOG_CRIT, "Configuration file \"%s\" parse error on line %ld (%s)\n",
			e.filename().c_str(), e.line(), e.message().c_str());
		exit(1);
	}

	string cameras;

	try
	{
		m_Delete = pt.get<bool>("maintenance.delete");
		m_Motion = pt.get<bool>("maintenance.motion");
		cameras = pt.get<string>("maintenance.cameras");
	}
	catch (const property_tree::ptree_error &e)
	{
		LOG(LOG_CRIT, "Configuration is invalid! Reason: %s\n", e.what());
		exit(1);
	}

	trim(cameras);

	vector<string> cameras_split;
	split(cameras_split, cameras, bind1st(equal_to<char>(), ','), token_compress_on);

	if (cameras.empty() || cameras_split.size() < 1)
	{
		LOG(LOG_CRIT, "Configuration is invalid! Reason: Must declare at least one camera.\n");
		exit(1);
	}

	for (vector<string>::iterator el = cameras_split.begin() ; el != cameras_split.end(); ++el)
	{
		int deleteafterdays, motionsensitivity, motionmaxdeviation, motioncontinuation;
		string motionmaskbitmap, destination;
		Mat motionmask;

		try
		{
			deleteafterdays = pt.get<int>(*el + ".deleteafterdays");
			motionsensitivity = pt.get<int>(*el + ".motionsensitivity");
			motionmaxdeviation = pt.get<int>(*el + ".motionmaxdeviation");
			motioncontinuation = pt.get<int>(*el + ".motioncontinuation");
			motionmaskbitmap = pt.get<string>(*el + ".motionmaskbitmap");
			destination = pt.get<string>(*el + ".destination");
		}
		catch (const property_tree::ptree_error &e)
		{
			LOG(LOG_CRIT, "Configuration is invalid! Reason: %s\n", e.what());
			exit(1);
		}

		trim(motionmaskbitmap);
		trim(destination);

		if (!motionmaskbitmap.empty())
		{
			if (!filesystem::exists(motionmaskbitmap))
			{
				LOG(LOG_CRIT, "Configuration is invalid! Reason: Mask bitmap file \"%s\" for camera \"%s\" not found.\n",
					motionmaskbitmap.c_str(), (*el).c_str());
				exit(1);
			}

			/*
			 * Use the "makemask" program to create and test masks. Masks work
			 * like everywhere else (white = include, black = exclude). Run
			 * "makemask" without arguments for further information.
			 */

			motionmask = imread(motionmaskbitmap, IMREAD_GRAYSCALE);

			if (motionmask.empty())
			{
				LOG(LOG_ERR, "Mask file \"%s\" could not be read.", motionmaskbitmap.c_str());
				exit(1);
			}

			motionmask = motionmask > 128; // Force mask to 1bpp/black and white
		}

		if (!filesystem::is_directory(destination))
		{
			LOG(LOG_CRIT, "Configuration is invalid! Reason: camera \"%s\" path \"%s\" does not exist.\n",
				(*el).c_str(), destination.c_str());
			exit(1);
		}

		camera cam;

		cam.deleteafterdays = deleteafterdays;
		cam.motionsensitivity = motionsensitivity;
		cam.motionmaxdeviation = motionmaxdeviation;
		cam.motioncontinuation = motioncontinuation;
		cam.name = *el;
		cam.destination = destination;
		cam.mask = motionmask;

		m_Cameras.push_back(cam);
	}
}

int video_motion_detection(const string& videofile, const camera& cam)
{
	VideoCapture capture = VideoCapture(videofile);

	if (!capture.isOpened())
	{
		LOG(LOG_ERR, "Video file \"%s\" could not be read.", videofile.c_str());
		return -1;
	}

	Mat prev_frame, current_frame, next_frame;

	capture >> prev_frame;
	cvtColor(prev_frame, prev_frame, COLOR_RGB2GRAY);
	try_apply_mask(prev_frame, cam.mask);

	capture >> current_frame;
	cvtColor(current_frame, current_frame, COLOR_RGB2GRAY);
	try_apply_mask(current_frame, cam.mask);

	capture >> next_frame;
	cvtColor(next_frame, next_frame, COLOR_RGB2GRAY);
	try_apply_mask(next_frame, cam.mask);

	// d1 and d2 for calculating the differences
	// number_of_changes, the amount of changes in the result matrix.

	Mat erode_kernel = getStructuringElement(MORPH_RECT, Size(2,2));

	int return_value = 0;
	int last_motion_at = 0;
	int number_of_sequence = 0;

	Mat d1, d2, motion;

	while (capture.grab())
	{
		prev_frame.release();
		prev_frame = current_frame;
		current_frame = next_frame;

		capture.retrieve(next_frame);
		cvtColor(next_frame, next_frame, COLOR_RGB2GRAY);
		try_apply_mask(next_frame, cam.mask);

		// Calculate the difference between the images and then do a bitwise AND.
		// Apply threshold and erode so that low differences, e.g. contrast change
		// due to sunlight or falling rain, are ignored.

		absdiff(prev_frame, next_frame, d1);
		absdiff(next_frame, current_frame, d2);
		bitwise_and(d1, d2, motion);

		d1.release();
		d2.release();

		threshold(motion, motion, 35, 255, THRESH_BINARY);
		erode(motion, motion, erode_kernel);

		/*
		 * The verbose output here will be something like
		 *
		 * [...]
		 * C0,C0,C0,C0,C0,C49,S1/3,C44,S2/3,C2,A,C64,S1/3,C67,S2/3,C9,A,
		 * C2,C0,C0,C0,C0,C115,S1/3,C136,S2/3,C134,S3/3,
		 * --- MOTION AT 127s ---
		 * C19,A,C18,C1,C0,C0,
		 * [...]
		 *
		 * The number after 'C' represents the number of changed pixels
		 * between the previous, current, and next frame.
		 *
		 * If the 'C' number is greater than the "motionsensitivity"
		 * setting, the program will start counting the frame sequence
		 * ("motioncontinuation" setting) and you will get an 'S' with
		 * two numbers; the first is the current number of frames that
		 * had had enough changed pixels. The second is how many frames
		 * with enough changes in pixels must occur in total before
		 * whatever is happening is actually considered to be motion.
		 *
		 * Once 'S' matches (e.g. 'S3/3' above), you will receive a
		 * "--- MOTION AT [...]s---" message with the video offset
		 * in seconds where motion occurred. These are limited to one
		 * message per second with motion.
		 *
		 * An 'A' means that motion has stopped; i.e. there were not
		 * enough changed pixels anymore and therefore frame counting
		 * was aborted.
		 *
		 * At the very the end, you get a log message like:
		 *
		 * INFO: Motion detection result for video file "[...]" was 18.
		 *
		 * The number at the end of this message indicates how many
		 * times motion according to the configured parameters has
		 * occurred in total for this video file.
		 *
		 * Unfortunately, for the time being, running this program in
		 * verbose mode is the only way to test changes made to the
		 * motion detection settings made in the "camsrv.ini" file.
		 */

		int number_of_changes =
			detect_motion(motion, cam.motionmaxdeviation);

		motion.release();

		if (m_Verbose) cout << 'C' << number_of_changes << ',' << flush;

		// If there are not enough changes over a large enough number of
		// frames, do not consider it to be motion. Otherwise, consider
		// it to be motiom and act on it.

		if(number_of_changes < cam.motionsensitivity)
		{
			if (number_of_sequence != 0)
			{
				if (m_Verbose) cout << "A," << flush;
				number_of_sequence = 0;
			}

			continue;
		}

		number_of_sequence++;

		if (m_Verbose) cout << 'S' << number_of_sequence << '/' << cam.motioncontinuation << ',' << flush;

		if(number_of_sequence >= cam.motioncontinuation)
		{
			int pos = capture.get(CAP_PROP_POS_MSEC) / 1000;

			if (pos > last_motion_at)
			{
				return_value++;
				last_motion_at = pos;

				if (m_Verbose) cout << endl << "--- MOTION AT " << pos << "s --- " << endl;
			}
		}
	}

	if (m_Verbose) cout << endl;

	prev_frame.release();
	current_frame.release();
	next_frame.release();
	capture.release();

	return return_value;
}

inline int detect_motion(const Mat& frame, int maxdeviation)
{
	// Check if there is motion, count the number of changes and return.

	// calculate the standard deviation
	Scalar mean, stddev;
	meanStdDev(frame, mean, stddev);

	// If the activity is spread all throughout the image, then it must
	// must not be genuine motion, but instead something like sun glare,
	// branches moving in the wind, or heavy snowfall.
	if(stddev[0] > maxdeviation)
		return 0;

	return countNonZero(frame == 255);

	/* The old code below is slower than using OpenCV's built-in way
	 * by about four seconds in verbose mode.
	 *
	 * It also produces slightly different results (number_of_changes
	 * is a little smaller), since it is wrongly incrementing by 2,
	 * about which the original author (Cédric Verstraeten) writes:
	 * "Increasing the i and j vars with 2 instead of 1, is just a
	 * simple heuristic to optimize the motion detection. For example
	 * if you would be processing very large images, you wouldn't need
	 * to iterate over every pixel."
	 *
	 * So it does half the work, but is slower... ;-)
	 *
	 * int number_of_changes = 0;
	 * for(int i = 0; i < frame.rows; i+=2) // height
	 * {
	 * 	for(int j = 0; j < frame.cols; j+=2) // width
	 * 	{
	 * 		// If the pixel at (i,j) is white (255), it is different
	 * 		// in the sequence prev_frame, current_frame, next_frame
	 * 		if(static_cast<int>(frame.at<uchar>(i,j)) == 255)
	 * 			number_of_changes++;
	 * 	}
	 * }
	 * return number_of_changes;
	 */
}

inline void try_apply_mask(Mat& matrix, Mat mask)
{
	// Compare with makemask.cpp

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

void LOG(int priority, const char *format, ...)
{
	if (m_Syslog)
	{
		va_list arglist;

		va_start(arglist, format);
		vsyslog(priority, format, arglist);
		va_end(arglist);
		return;
	}

	string loglevel;

	switch(priority)
	{
		case LOG_ALERT:		loglevel = "ALERT";		break;
		case LOG_CRIT:		loglevel = "CRIT";		break;
		case LOG_DEBUG:		loglevel = "DEBUG";		break;
		case LOG_EMERG:		loglevel = "EMERG";		break;
		case LOG_ERR:		loglevel = "ERR";		break;
		case LOG_INFO:		loglevel = "INFO";		break;
		case LOG_NOTICE:	loglevel = "NOTICE";	break;
		case LOG_WARNING:	loglevel = "WARNING";	break;
		default:			loglevel = "UNKNOWN";	break;
	}

	printf("%s: ", loglevel.c_str());

	va_list arglist;

	va_start(arglist, format);
	vprintf(format, arglist);
	printf("\n");
	va_end(arglist);
}
