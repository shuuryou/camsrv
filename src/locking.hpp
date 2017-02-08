#ifndef LOCKING_HPP
#define LOCKING_HPP

#include <fcntl.h>
#include <stdio.h>

#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

int check_if_running_and_lock(const string &lockfile);
void delete_lockfile();

#endif
