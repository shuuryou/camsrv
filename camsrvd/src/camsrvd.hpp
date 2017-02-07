/*
 * camsrvd - Supervisory Daemon for Camera Stream Grabbing
 *
 */

#ifndef CAMSRVD_HPP
#define CAMSRVD_HPP

#include <algorithm>

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "locking.hpp"

extern "C"
{
	#include "nargv/nargv.h"
}

using namespace std;
using namespace boost;

#define LOGGER_EXECUTABLE "/usr/bin/logger"
#define LOGGER_TAG "camsrvd"
#define LOGGER_PRIORITY "user.notice"
#define LOCKFILE "/var/run/camsrvd.pid"

#define SENDMAIL_EXECUTABLE "/usr/lib/sendmail -t"

typedef struct camsrvdcamera
{
	string name;
	string command;
	pid_t pid;
	int errcount;
	bool disabled;
	bool resetting;
	time_t lastreset;
	time_t laststart;
} camera;

int main (int argc, const char* argv[]);

void notify_camera_disabled(const camera disabledtask);

void load_settings(const string& filename);

void setup_signal_handler();
void handle_signal(int signum);

void become_daemon();

void redirect_output_to_logger();
void close_logger();

void output_statistics();

pid_t run_process(string cmdline);

void posix_fail(string why, bool terminate);
#endif
