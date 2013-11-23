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
#include "pfs_data.h"
#include "pfs_localfile.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

int do_task_after(t_pfs_tasklist *task);

int do_error_file_sync(int fd, t_pfs_tasklist *task, int status_e);

int do_ok_file_sync(int fd, t_pfs_tasklist *task);

int init_data_task(t_pfs_tasklist ** tasklist, t_task_base *base, t_task_sub *sub);

void do_send_task_head(uint8_t cmdid, uint8_t status, t_pfs_tasklist *task, pfs_cs_peer *peer);

int do_addfile_task(int fd, t_pfs_sig_head *h, uint8_t *status, t_task_base *base);

int do_delfile_task(uint8_t *status, t_task_base *base);

void do_send_task(int fd, t_task_base *base, t_pfs_sig_head *h);

int do_recv_task(int fd, t_pfs_sig_head *h, t_task_base *base);

int do_push_task(int fd, t_pfs_sig_head *h, t_task_base *base);

void create_push_rsp(t_pfs_tasklist *task, int fd);

void create_push_rsp_err(char *errmsg, int fd);

#endif
