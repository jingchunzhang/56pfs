/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef __PFS_SIG_TASK_Q_H_
#define __PFS_SIG_TASK_Q_H_
#include "list.h"
#include "global.h"
#include "common.h"
#include "protocol.h"
#include "pfs_init.h"
#include "pfs_task.h"
#include "pfs_up.h"
#include "pfs_localfile.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

int do_recv_task(int fd, t_pfs_sig_head *h, t_task_base *base);

int do_push_task(int fd, t_pfs_sig_head *h, t_task_base *base);

void create_push_rsp(t_pfs_tasklist *task, int fd);

void create_push_rsp_err(char *errmsg, int fd);

#endif
