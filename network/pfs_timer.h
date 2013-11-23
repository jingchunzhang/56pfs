/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef __PFS_CB_H_
#define __PFS_CB_H_

#include <time.h>
#include "list.h"
typedef struct 
{
	void (*cb)(char *);
	char args[256];
	time_t next_time;
	time_t span_time;
	int loop;    /*0 only once, 1 loop for ever */
} t_pfs_timer;

typedef struct 
{
	t_pfs_timer pfs_timer;
	list_head_t tlist;
} t_pfs_timer_list;

int add_to_delay_task(t_pfs_timer *pfs_timer);

void scan_delay_task();

int delay_task_init();
#endif
