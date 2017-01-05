/*
 * camsrvd - Supervisory Daemon for Camera Stream Grabbing
 *
 */

#ifndef CAMSRVD_HPP
#define CAMSRVD_HPP

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

extern "C"
{
	#include "nargv/nargv.h"
}

using namespace std;
using namespace boost;

#define LOGGER_EXECUTABLE "/usr/bin/logger"
#define LOGGER_TOPIC "camsrvd"
#define LOCKFILE "/var/run/camsrvd.pid"

typedef struct camsrvdcamera
{
	string name;
	string command;
	pid_t pid;
	int errcount;
	bool disabled;
	time_t laststart;
} camera;

int main (int argc, const char* argv[]);

void notify_camera_disabled(const camera disabledtask);

void load_settings(const string& filename);

void setup_signal_handler();
void handle_signal(int signum);

void check_if_running_and_lock();
void delete_lockfile();

void become_daemon();

void redirect_output_to_logger();
void close_logger();

pid_t run_process(string cmdline);

void posix_fail(string why, bool terminate);

#endif