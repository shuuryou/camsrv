/*
 * camsrvd - Supervisory Daemon for Camera Stream Grabbing
 *
 */

#include <camsrvd.hpp>

int		m_LoggerPipe[2];
pid_t	m_LoggerPID;

string	m_MailTo;
string 	m_SendMsg;
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

	check_if_running_and_lock();

	// Required for infinite sleep
	sigset_t empty_signalset;
	sigemptyset(&empty_signalset);

	// Launch the instances
	printf("Starting commands for cameras.\n");

	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		printf("Starting camera \"%s\".\n", cam->name.c_str());

		cam->pid = run_process(cam->command);
		cam->errcount = 0;
		cam->disabled = false;
		cam->laststart = time(NULL);
	}

	printf("Starting command monitoring.\n");

	do
	{
		bool all_cameras_disabled = true;
		bool may_suspend = true;

		for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
		{
			if (cam->disabled)
				continue;

			all_cameras_disabled = false;

			if (cam->pid == -1 || kill(cam->pid, 0) == -1)
			{
				may_suspend = false;
				cam->errcount++;

				printf("Camera \"%s\" with PID %d is no longer running. It has termined unexpectedly %d time(s) before.\n",
					cam->name.c_str(), cam->pid, cam->errcount);

				sleep(m_ResetTimer);

				if (cam->errcount > m_MaxFailures)
				{
					cam->disabled = true;
					printf("Camera \"%s\" has failed too many times and has been disabled.\n", cam->name.c_str());
				}
				else
				{
					printf("Attempting to recover camera \"%s\"...\n", cam->name.c_str());
					cam->pid = run_process(cam->command);
					cam->laststart = time(NULL);
				}
			}
			else if (cam->errcount != 0)
			{
				may_suspend = false;
				time_t now = time(NULL);

				if (now - cam->laststart > m_ResetTimer)
				{
					printf("Camera \"%s\"appears to be working fine again, so resetting error count.\n", cam->name.c_str());
					cam->errcount = 0;
				}
			}
		}

		if (all_cameras_disabled)
		{
			printf("All cameras have become disabled. There is nothing left to do. Terminating.\n");
			break;
		}

		// Wait for something to do. If we must reset the error count, we may only
		// sleep for a limited time. Otherwise, we can sleep indefinitely.
		if (!may_suspend)
		{
			sleep(60);
			goto end;
		}

		printf("Suspending until something happens.\n");
		sigsuspend(&empty_signalset);
		printf("Something has happened.\n");

	end:
		if (TERMINATE)
			break;

	} while (true);

	bool do_kill = false;

again:
	bool repeat = false;

	for (vector<camera>::iterator cam = m_Cameras.begin() ; cam != m_Cameras.end(); ++cam)
	{
		if (cam->disabled)
			continue;

		if (cam->pid == -1 || kill(cam->pid, 0) == -1)
			continue; // Not running

		if (do_kill)
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

		repeat = true;
	}

	if (repeat && !do_kill)
	{
		repeat = false;
		do_kill = true;

		printf("Giving all camera processes some time to exit.\n");
		uint remaining = 5;

		while (remaining > 0)
			remaining = sleep(remaining);

		goto again;
	}

	printf("Terminating.\n");

	return 0;
}

void notify_camera_disabled(const camera camdis)
{
	pid_t pid = fork();

	if (pid == -1)
	{
		posix_fail("Could not fork to execute message sending program.", false);
		return;
	}

	if (pid == 0)
	{
		/* Child here */
		int ret = execl(m_SendMsg.c_str(), m_SendMsg.c_str(),
			m_MailTo.c_str(), camdis.name.c_str(), camdis.command.c_str(), NULL);

		if (ret == -1)
			posix_fail("Executing for message sending program failed.", false);
	}

	/* Parent here */
	int retval;
	waitpid(pid, &retval, 0);

	if (retval != 0)
	{
		fprintf(stderr, "Unable to send notification. Message sending program returned %d.\n", retval);
		return;
	}

	printf("Successfully sent notification. Message sending program returned %d.\n", retval);
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
		m_SendMsg = pt.get<string>("camsrvd.sendmsg");
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
	trim(m_SendMsg);
	trim(m_CommandTpl);
	trim(m_FilenameTpl);

	trim(cameras);

	vector<string> cameras_split;

	split(cameras_split, cameras, bind1st(equal_to<char>(), ','), token_compress_on);

	if (m_MailTo.length() == 0)
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing mail recipient.");
		exit(1);
	}
	else if (m_SendMsg.length() == 0)
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing command template.");
		exit(1);
	}
	else if (m_CommandTpl.length() == 0)
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing command template.");
		exit(1);
	}
	else if (m_FilenameTpl.length() == 0)
	{
		fprintf(stderr, "Configuration is invalid! Reason: missing filename template.");
		exit(1);
	}
	else if (cameras.length() == 0 || cameras_split.size() < 1)
	{
		fprintf(stderr, "Configuration is invalid! Reason: must declare at least one camera.");
		exit(1);
	}

	if (!filesystem::exists(m_SendMsg))
	{
		fprintf(stderr, "Configuration is invalid! Reason: Message sending program not found.");
		exit(1);
	}

	{
		struct stat st;
		if (stat(m_SendMsg.c_str(), &st) < 0 || (st.st_mode & S_IEXEC) == 0)
		{
			fprintf(stderr, "Configuration is invalid! Reason: Message sending program not executable.");
			exit(1);
		}
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
			/* Reserved */
			break;
	}
}

void check_if_running_and_lock()
{
	int fd = open(LOCKFILE, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	if (fd < 0)
		posix_fail("Unable to open lock file.", true);

	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fd, F_SETLK, &fl) < 0)
	{
		if (errno == EACCES || errno == EAGAIN)
		{
			close(fd);
			fprintf(stderr, "Another instance is already running.\n");
			exit(1);
		}

		posix_fail("Could not acquire lock on lock file.", true);
	}

	char pid[16];
	ftruncate(fd, 0);
	sprintf(pid, "%ld\n", (long)getpid());
	write(fd, pid, strlen(pid));

	atexit(delete_lockfile);
}

void delete_lockfile()
{
	if (!filesystem::exists(LOCKFILE))
		return;

	unlink(LOCKFILE);
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

		if (execl(LOGGER_EXECUTABLE, LOGGER_EXECUTABLE, "-t", LOGGER_TOPIC, NULL) == -1)
			posix_fail("Starting logger failed.", true);
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

	waitpid(m_LoggerPID, &ret, 0); // Logger exists once its stdin closes

	// stdout and stderr are still closed from here. Caller has to fix this.
}

pid_t run_process(string cmdline)
{
	// Run a process in the background and return its PID.

	if (cmdline.length() == 0)
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

	if (errno == 0)
		return;

	fprintf(stderr, "Failure: %s - %s\n", why.c_str(), strerror(errno));

	if (terminate)
		exit(1);
}

