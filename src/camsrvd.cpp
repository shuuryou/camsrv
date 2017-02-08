/*
 * camsrvd - Supervisory Daemon for Camera Stream Grabbing
 *
 */

#include "camsrvd.hpp"

int		m_LoggerPipe[2];
pid_t	m_LoggerPID;

string	m_MailTo;
string	m_CommandTpl;
string	m_FilenameTpl;

int		m_MaxFailures;
int		m_ResetTimer;

vector<camera> m_Cameras;

bool	TERMINATE;

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Missing configuration file or too many arguments.\n");
		return 1;
	}

	load_settings(argv[1]);
	setup_signal_handler();

	become_daemon();

	// Unfortunately we can only create a lock file after we
	// have become a daemon, since the checking and creating
	// operation is an atomic operation to avoid race conditions.
	int lock_status = check_if_running_and_lock(LOCKFILE);

	switch (lock_status)
	{
		case -2:
			abort(); // Must never happen
		case -1:
			posix_fail("Could not create lock file.", true);
		case  1: 
			fprintf(stderr, "Another instance is already running.\n");
			exit(1);
		case  0:
			// Happy days.
			break;
	}

	// Required for infinite sleep
	sigset_t empty_signalset;
	sigemptyset(&empty_signalset);

	// Launch the instances
	printf("Starting commands for cameras.\n");

	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		printf("Starting camera \"%s\".\n", cam->name.c_str());

		cam->laststart = time(NULL);
		cam->pid = run_process(cam->command);
	}

	printf("Starting command monitoring.\n");

	while (!TERMINATE)
	{
		bool all_cameras_disabled = true;
		unsigned int sleep_time = 0;

		for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
		{
			if (cam->disabled)
				continue; // for

			time_t now = time(NULL);

			if (!cam->resetting && (cam->pid == -1 || kill(cam->pid, 0) == -1))
			{
				cam->errcount++;

				printf("Camera \"%s\" with PID %d is no longer running. It has termined unexpectedly %d time(s) before.\n",
					cam->name.c_str(), cam->pid, cam->errcount);
				
				if (cam->errcount >= m_MaxFailures)
				{
					cam->disabled = true;
					printf("Camera \"%s\" has failed too many times and has been disabled.\n", cam->name.c_str());
					notify_camera_disabled(*cam);
					
					continue; // for
				}
				
				cam->resetting = true;
				cam->lastreset = time(NULL);
			}
			else if (cam->resetting && now - cam->lastreset >= m_ResetTimer)
			{
				assert(cam->lastreset != (time_t)-1);
				
				printf("Attempting to recover camera \"%s\"...\n", cam->name.c_str());
				cam->laststart = now;
				cam->resetting = false;
				cam->lastreset = (time_t)-1;
				cam->pid = run_process(cam->command);
			}
			else if (cam->errcount != 0 && (now - cam->laststart >= m_ResetTimer))
			{
				printf("Camera \"%s\"appears to be working fine again, so resetting error count.\n", cam->name.c_str());
				cam->errcount = 0;
			}
			
			all_cameras_disabled = false;
			
			if (cam->resetting || cam->errcount != 0)
			{
				// Figure out how long to sleep until something needs to
				// be reset or restarted.
				unsigned int tmp;
				
				if (cam->resetting)
					tmp = (unsigned int)(m_ResetTimer - (now - cam->lastreset));
				else if (cam->errcount != 0)
					tmp = (unsigned int)(m_ResetTimer - (now - cam->laststart));
				
				if (sleep_time == 0)
					sleep_time = tmp;
				else
					sleep_time = min(sleep_time, tmp);
			}
		}

		if (all_cameras_disabled)
		{
			printf("All cameras have become disabled. There is nothing left to do.\n");
			break; // while
		}
		
		if (sleep_time == 0)
		{
			printf("Suspending until something happens.\n");
			sigsuspend(&empty_signalset);
			printf("Something has happened.\n");
		}
		else
		{
			printf("Sleeping for %d seconds or less.\n", sleep_time);
			sleep(sleep_time); 
			printf("Woke up from sleep.\n");
		}
	}

	// Terminate processes, wait 5 seconds, then try to kill remaining
	// processes up to 10 times, waiting 5 seconds between attempts.
	bool send_sigkill = false;
	bool terminated_something;
	
	for (int sentinel = 10; sentinel >= 0; sentinel--)
	{
		terminated_something = false;
		
		for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
		{
			if (cam->disabled)
				continue; // for

			if (cam->pid == -1 || kill(cam->pid, 0) == -1) // Not running
				continue; // for

			if (send_sigkill)
			{
				printf("Killing camera \"%s\" process with PID %d.\n", cam->name.c_str(), cam->pid);

				if (kill(cam->pid, SIGKILL) == -1)
					posix_fail("Unable to kill process.", false);
			}
			else
			{
				printf("Terminating camera \"%s\" process with PID %d.\n", cam->name.c_str(), cam->pid);

				if (kill(cam->pid, SIGTERM) == -1)
					posix_fail("Unable to terminate process.", false);
			}
			
			terminated_something = true;
		}
		
		if (!terminated_something) // We are done!
			break; // for
		
		printf("Giving all camera processes some time to exit.\n");

		for (uint remaining = 5; remaining > 0;)
			remaining = sleep(remaining);
		
		 // If we sent SIGTERM, the next round is with SIGKILL
		if (!send_sigkill) send_sigkill = true;
	}

	printf("Terminating.\n");

	return 0;
}

void notify_camera_disabled(const camera camdis)
{
	// The following was shoplifted from the mdadm version 4.0
	// "Monitor.c" file (static void alert). I therefore assume
	// that the code is solid.

	pid_t pid = fork();
	
	if (pid == -1)
	{
		posix_fail("Could not fork to execute sendmail.", false);
		return;
	}
	
	if (pid == 0)
	{
		/* Child here */

		FILE *mp = popen(SENDMAIL_EXECUTABLE, "w");

		if (!mp)
			posix_fail("Could not execute sendmail to send an alert.", true);

		signal(SIGPIPE, SIG_IGN);
		
		char hname[256];
		gethostname(hname, sizeof(hname));

		fprintf(mp, "From: CamSrv <root>\n");
		fprintf(mp, "To: %s\n", m_MailTo.c_str());
		fprintf(mp, "Subject: Problem with camera \"%s\" on host \"%s\"\n\n",
			camdis.name.c_str(), hname);

		fprintf(mp, "This is an automatically generated notification from camsrvd running on \"%s\".\n\n", hname);
		fprintf(mp, "The following camera has been disabled because its command has failed too many times:\n\n");

		fprintf(mp, "%s\n", camdis.name.c_str());
		fprintf(mp, "%s\n\n", camdis.command.c_str());

		fprintf(mp, "You may wish to investigate what is going on. Further information might be available in the syslog.\n\n");
		fprintf(mp, "Apologies for any inconvenience this may cause. For resolving any issues I can only say, Godspeed.\n");

		pclose(mp);
		
		exit(0);
	}
	
	/* Parent here */
	int retval;
	waitpid(pid, &retval, 0);

	if (retval != 0)
	{
		fprintf(stderr, "Unable to send notification. Sendmail returned %d.\n", retval);
		return;
	}

	printf("Successfully sent notification. Sendmail returned %d.\n", retval);
}

void load_settings(const string& filename)
{
	if (!filesystem::exists(filename))
	{
		fprintf(stderr, "Configuration file \"%s\" not found.\n", filename.c_str());
		exit(1);
	}

	property_tree::ptree pt;

	try
	{
		property_tree::ini_parser::read_ini(filename, pt);
	}
	catch (const property_tree::ini_parser::ini_parser_error &e)
	{
		fprintf(stderr, "Configuration file \"%s\" parse error on line %ld (%s)\n",
			e.filename().c_str(), e.line(), e.message().c_str());
		exit(1);
	}

	string cameras;

	try
	{
		m_MailTo = pt.get<string>("camsrvd.mailto");
		m_CommandTpl = pt.get<string>("camsrvd.commandtpl");
		m_FilenameTpl = pt.get<string>("camsrvd.filenametpl");

		m_MaxFailures = pt.get<int>("camsrvd.maxfailures");
		m_ResetTimer = pt.get<int>("camsrvd.resettimer");

		cameras = pt.get<string>("camsrvd.cameras");
	}
	catch (const property_tree::ptree_error &e)
	{
		fprintf(stderr, "Configuration is invalid! Reason: %s\n", e.what());
		exit(1);
	}

	trim(m_MailTo);
	trim(m_CommandTpl);
	trim(m_FilenameTpl);

	trim(cameras);

	vector<string> cameras_split;

	split(cameras_split, cameras, bind1st(equal_to<char>(), ','), token_compress_on);

	if (m_MailTo.empty())
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing mail recipient.");
		exit(1);
	}
	else if (m_CommandTpl.empty())
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing command template.");
		exit(1);
	}
	else if (m_FilenameTpl.empty())
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing filename template.");
		exit(1);
	}
	else if (cameras.empty() || cameras_split.size() < 1)
	{
		fprintf(stderr, "Configuration is invalid! Reason: must declare at least one camera.");
		exit(1);
	}

	for (vector<string>::iterator el = cameras_split.begin() ; el != cameras_split.end(); ++el)
	{
		string stream, destination;

		try
		{
			stream = pt.get<string>(*el + ".stream");
			destination = pt.get<string>(*el + ".destination");
		}
		catch (const property_tree::ptree_error &e)
		{
			fprintf(stderr, "Configuration is invalid! Reason: %s\n", e.what());
			exit(1);
		}

		if (!filesystem::is_directory(destination))
		{
			fprintf(stderr, "Configuration is invalid! Reason: camera \"%s\" path \"%s\" does not exist.\n",
				(*el).c_str(), destination.c_str());
			exit(1);
		}

		{
			filesystem::path p = filesystem::canonical(destination);
			p /= m_FilenameTpl; // yes, this fucked up syntax is correct for path.join() *sigh*
			destination = p.string();
		}

		// Now we build the command from the template
		// TODO: Is it REALLY okay like this?

		camera cam;

		replace_all(stream, "\"", "\\\"");
		replace_all(destination, "\"", "\\\"");

		cam.name = *el;
		cam.command = m_CommandTpl;
		replace_all(cam.command, "{STREAM}", "\"" + stream + "\"");
		replace_all(cam.command, "{DESTINATION}", "\"" + destination + "\"");
		cam.pid = (pid_t)-1;
		cam.errcount = 0;
		cam.disabled = false;
		cam.resetting = false;
		cam.lastreset = (time_t)-1;
		cam.laststart = (time_t)-1; 

		m_Cameras.push_back(cam);
	}
}

void setup_signal_handler()
{
	struct sigaction sigact;

	sigact.sa_handler = handle_signal;
	sigact.sa_flags = 0;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGUSR1);
	sigact.sa_mask = mask;

	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);
}

void handle_signal(int signum)
{
	switch (signum)
	{
		case SIGTERM:
			if (!TERMINATE)
				TERMINATE = true;
			break;
		case SIGCHLD:
			pid_t pid;
			int retval;

			while((pid = waitpid(-1, &retval, WNOHANG)) > 0);
			break;
		case SIGUSR1:
			output_statistics();
			break;
	}
}

void become_daemon()
{
	pid_t pid;
	struct rlimit rl;

	// Clear file creation mask
	umask(0);

	// Get maximum number of file descriptors.
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		posix_fail("Cannot get file limit.", true);

	// Become a session leader to lose controlling TTY.
	if ((pid = fork()) < 0)
		posix_fail("Could not fork to become daemon.", true);

	if (pid != 0)
	{
		/* Parent here */
		exit(0);
	}

	/* Child here */

	setsid();

	// Ensure future opens won't allocate controlling TTYs.
	{
		struct sigaction sa;
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;

		if (sigaction(SIGHUP, &sa, NULL) < 0)
			posix_fail("Could not ignore SIGHUP.", true);
	}

	/* Child now forks again */

	if ((pid = fork()) < 0)
		posix_fail("Could not fork the second time to become daemon.", true);

	if (pid != 0)
	{
		/* Parent here */
		exit(0);
	}

	/* Child here */

	// Change the current working directory to the root so
	// we won't prevent file systems from being unmounted.

	if (chdir("/") < 0)
		posix_fail("Could not change working directory to root directory.", true);

	// Close all open file descriptors.
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;

	for (rlim_t i = 0; i < rl.rlim_max; i++)
		close(i);

	// stdin comes from /dev/null, stdout and stderr go to /dev/null
	{
		int devnull = open("/dev/null", O_RDWR);

		int ret;

		ret = dup2(devnull, STDIN_FILENO);

		if (ret == -1)
			posix_fail("Unable to duplicate /dev/null to standard input.", true);

		ret = dup2(devnull, STDOUT_FILENO);

		if (ret == -1)
			posix_fail("Unable to duplicate /dev/null to standard output.", true);

		ret = dup2(devnull, STDERR_FILENO);

		if (ret == -1)
			posix_fail("Unable to duplicate /dev/null to standard error.", true);
	}

	// Standard output and standard error go to logger from here
	redirect_output_to_logger();
}

void redirect_output_to_logger()
{
	// Kludge to redirect stdout and stderr to syslog. This is required so that
	// the processes that are run output all of their shit to syslog. As a side-
	// effect, we don't need to bother with calling syslog functions.

	if (pipe(m_LoggerPipe) != 0)
		posix_fail("Unable to create a pipe for logger.", true);

	m_LoggerPID = fork();

	if (m_LoggerPID == -1)
		posix_fail("Could not fork to start logger.", true);

	if (m_LoggerPID == 0)
	{
		/* Child here */

		// Child inherits /dev/null as stdout and stderr from become_daemon()

		close(m_LoggerPipe[1]); // Close child's writing end of the pipe

		dup2(m_LoggerPipe[0], STDIN_FILENO); // Child's stdin becomes the pipe
		close(m_LoggerPipe[0]); // OK as parent still has it open

		if (execl(LOGGER_EXECUTABLE, LOGGER_EXECUTABLE,
			"-t", LOGGER_TAG, "-p", LOGGER_PRIORITY, NULL) == -1)
		{
			posix_fail("Starting logger failed.", true);
		}
	}

	/* Parent here */

	close(m_LoggerPipe[0]); // Close parent's reading end of the pipe

	int ret;

	ret = dup2(m_LoggerPipe[1], STDOUT_FILENO);
	if (ret == -1) posix_fail("Unable to duplicate logger file descriptor to stdout.", true);

	ret = dup2(m_LoggerPipe[1], STDERR_FILENO);
	if (ret == -1) posix_fail("Unable to duplicate logger file descriptor to stderr.", true);

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	atexit(close_logger);
}

void close_logger()
{
	int ret;

	close(m_LoggerPipe[1]);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	waitpid(m_LoggerPID, &ret, 0); // Logger exits once its stdin closes

	// stdout and stderr are still closed from here. Caller has to fix this.
}

void output_statistics()
{
	char buf[255], buf2[255];
	struct tm *info;
	
	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		info = localtime(&cam->lastreset);
		strftime(buf, 255,"%x %X", info);
		
		info = localtime(&cam->laststart);
		strftime(buf2, 255,"%x %X", info);
		
		printf("Camera \"%s\" - Command: \"%s\", PID: %d, Error count: %d, Disabled? %s, Resetting? %s, Last reset: %s, Last start: %s.\n",
			cam->name.c_str(), cam->command.c_str(), cam->pid, cam->errcount,
			cam->disabled ? "Yes" : "No",
			cam->resetting ? "Yes" : "No",
			buf, buf2);
	}
}

pid_t run_process(string cmdline)
{
	// Run a process in the background and return its PID.

	if (cmdline.empty())
	{
		fprintf(stderr, "Refusing to run an empty command line.");
		return -1;
	}

	// TODO: nargv is plain C and I don't know how to use smart pointers :(
	NARGV *nargv = nargv_parse(const_cast<char*>(cmdline.c_str()));

	// from here nargv holds memory

	if (nargv->error_code)
	{
		fprintf(stderr, "Unable to parse command line \"%s\" (%d) %s\n",
			cmdline.c_str(), nargv->error_code, nargv->error_message);

		nargv_free(nargv);
		return -1;
	}

	if (nargv->argc == 0)
	{
		fprintf(stderr, "Command line is not empty, but parsed to nothing. WTF?");
		nargv_free(nargv);
		return -1;
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		posix_fail("Could not fork to start process.", false);
		nargv_free(nargv);
		return -1;
	}

	if (pid == 0)
	{
		/* Child here */
		if (execv(nargv->argv[0], nargv->argv) == -1)
			posix_fail("Starting process failed.", true);
	}

	/* Parent here */
	printf("Started \"%s\" with PID %d.\n", nargv->argv[0], pid);

	nargv_free(nargv);

	return pid;
}

void posix_fail(string why, bool terminate)
{
	// Deal with all the stupid UNIX API calls that can fail.
	// This is a lot like those dumb VB6 error handlers.

	if (errno == 0)
		return;

	fprintf(stderr, "Failure: %s - %s\n", why.c_str(), strerror(errno));

	if (terminate)
		exit(1);
}
