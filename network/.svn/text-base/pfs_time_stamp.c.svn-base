/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "log.h"
#include "myconfig.h"
#include "pfs_time_stamp.h"
#include "pfs_timer.h"
#include "pfs_init.h"

#include <stdio.h>

extern int glogfd;

static int fd_time_stamp = -1;
extern int tmp_status_fd;

typedef struct {
	time_t dirtime;
} t_time_stamp;

static void sync_fd(char *arg)
{
	fsync(tmp_status_fd);
	fsync(fd_time_stamp);
}

int init_pfs_time_stamp()
{
	char *datadir = myconfig_get_value("pfs_path");
	if (datadir == NULL)
		datadir = "../data";
	char tmpfile[256] = {0x0};
	snprintf(tmpfile, sizeof(tmpfile), "%s/pfs_time_stamp", datadir);
	struct stat filestat;
	if (stat(tmpfile, &filestat))
		LOG(glogfd, LOG_ERROR, "%s not exit init it!\n", tmpfile);

	fd_time_stamp = open(tmpfile, O_CREAT | O_RDWR | O_LARGEFILE, 0644);
	if (fd_time_stamp < 0)
	{
		LOG(glogfd, LOG_ERROR, "open %s err %m\n", tmpfile);
		return -1;
	}
	if (ftruncate(fd_time_stamp, sizeof(t_time_stamp) * DIR1 * DIR2))
	{
		LOG(glogfd, LOG_ERROR, "ftruncate %s err %m\n", tmpfile);
		return -1;
	}
	t_pfs_timer pfs_timer;
	memset(&pfs_timer, 0, sizeof(pfs_timer));
	pfs_timer.span_time = 600;
	pfs_timer.cb = sync_fd;
	pfs_timer.loop = 1;
	if (add_to_delay_task(&pfs_timer))
		LOG(glogfd, LOG_ERROR, "add delay task err %m\n");
	return 0;
}

int set_time_stamp_by_int(int dir1, int dir2, time_t tval)
{
	off_t pos = (dir1 * DIR2 + dir2) * sizeof(t_time_stamp);
	off_t n = lseek(fd_time_stamp, pos, SEEK_SET);
	if (n != pos)
	{
		LOG(glogfd, LOG_ERROR, "lseek err %m\n");
		return -1;
	}

	n = write(fd_time_stamp, &tval, sizeof(tval));
	if (n != sizeof(tval))
	{
		LOG(glogfd, LOG_ERROR, "write err %d:%d %m\n", n, sizeof(tval));
		return -1;
	}
	return 0;
}

int set_time_stamp(t_task_base *task)
{
	int d1, d2;
	if (get_dir1_dir2(task->filename, &d1, &d2))
		return -1;
	return set_time_stamp_by_int(d1, d2, task->mtime);
}

time_t get_time_stamp_by_int(int dir1, int dir2)
{
	int pos = (dir1 * DIR2 + dir2) * sizeof(time_t);
	int n = lseek(fd_time_stamp, pos, SEEK_SET);
	if (n < 0)
	{
		LOG(glogfd, LOG_ERROR, "lseek err %m\n");
		return -1;
	}

	time_t val = 0;
	n = read(fd_time_stamp, &val, sizeof(val));
	if (n != sizeof(val))
	{
		LOG(glogfd, LOG_ERROR, "read err %d:%d %m\n", n, sizeof(val));
		return -1;
	}
	return val;
}

void close_all_time_stamp()
{
	if (fd_time_stamp > 0)
		close(fd_time_stamp);
}

