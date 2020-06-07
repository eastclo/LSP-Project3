#ifndef DEFAULT_H_
#define DEFAULT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <syslog.h>
#include <signal.h>

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef FILELEN 
	#define FILELEN 256 
#endif
#ifndef BUFLEN
	#define BUFLEN 1024
#endif
#ifndef SECOND_TO_MICRO 
	#define SECOND_TO_MICRO 1000000
#endif

#endif
