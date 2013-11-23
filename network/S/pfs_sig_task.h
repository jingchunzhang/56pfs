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
#include "pfs_sig.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

int do_newtask(int fd, t_pfs_sig_head *h, t_pfs_sig_body *b);

void do_task_rsp(t_pfs_tasklist *task);

int sync_task_2_domain(t_pfs_tasklist *task);

void do_sync_dir_req(int fd, t_pfs_sig_body *b);

int do_dispatch_task(char *sip, t_pfs_tasklist *task, pfs_cs_peer *peer, uint8_t status);

void do_sync_del_req(int fd, t_pfs_sig_body *b);

#endif
