#include "locking.hpp"

string m_LockingLockfile;

int check_if_running_and_lock(const string& lockfile)
{
	/*
	 * Return values:
	 * -2 Invalid argument supplied to lockfile
	 * -1 POSIX error occurred
	 *  0 Process is not running yet
	 *  1 Another process has locked the lockfile
	 */

	if (lockfile.empty())
		return -2;

	int fd = open(lockfile.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	if (fd < 0)
		return -1;

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
			return 1;
		}

		return -1;
	}

	char pid[16];
	ftruncate(fd, 0);
	sprintf(pid, "%ld\n", (long)getpid());
	write(fd, pid, strlen(pid));

	m_LockingLockfile = lockfile;

	atexit(delete_lockfile);

	return 0;
}

void delete_lockfile()
{
	if (m_LockingLockfile.empty() || !filesystem::exists(m_LockingLockfile))
		return;

	unlink(m_LockingLockfile.c_str());
}