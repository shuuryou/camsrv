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

#ifndef MAINTENANCE_HPP
#define MAINTENANCE_HPP

#include <string>

#include <assert.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <opencv2/opencv.hpp>

#include "locking.hpp"

#define LOCKFILE "/var/lock/camsrvd-maintenance.pid"
#define SYSLOG_IDENT "camsrv-maintenance"

using namespace std;
using namespace boost;
using namespace cv;

typedef struct maintenancecamera
{
	int deleteafterdays;
	int motionsensitivity;
	int motionmaxdeviation;
	int motioncontinuation;
	string name;
	string destination;
	Mat mask;
} camera;

int main (int argc, char* const argv[]);
void exit_usage(const char* argv0);
void do_delete();
void do_motion();
void load_settings(const string& filename);
int video_motion_detection(const string& videofile, const camera& cam);
int detect_motion(const Mat& frame, int max_deviation);
void try_apply_mask(Mat& matrix, Mat mask);
void LOG(int priority, const char *format, ...);
#endif
