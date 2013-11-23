/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef _PFS_TIME_STAMP_H_
#define _PFS_TIME_STAMP_H_
#include "pfs_task.h"

int init_pfs_time_stamp();

void close_all_time_stamp();

int set_time_stamp(t_task_base *task);

time_t get_time_stamp_by_int(int dir1, int dir2);

int set_time_stamp_by_int(int dir1, int dir2, time_t tval);
#endif
