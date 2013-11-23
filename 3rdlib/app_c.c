/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "c_api.h"
#include "c_api_dog.h"
void EnterDaemonMode(void)
{
	switch(fork())
	{
		case 0:

			break;

		case -1:

			fprintf(stderr, "fork error %m\n");
			exit(-1);
			break;

		default:

			exit(0);
			break;
	}

	setsid();
}

int main(int argc, char **argv)
{
	EnterDaemonMode();

	T_WATCH_DOG val;
	memset(&val, 0, sizeof(val));
	val.intval = 30;
	val.flag = 1;
	val.last = time(NULL);
	readlink("/proc/self/exe", val.cmd, 4095);
	int ret = reg_process(getpid(), &val);
	if (ret != 0)
		fprintf(stderr, "reg_process error %m!\n");

	while (1)
	{
		touch_process();
		SetInt(0x04987788, 0x12345678);
		sleep(5);
	}

	return 0;
}
