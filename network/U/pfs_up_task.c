/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_up_task.h"
#include "pfs_up.h"
#include "pfs_task.h"
#include "common.h"
#include "global.h"
#include "pfs_so.h"
#include "myepoll.h"
#include "protocol.h"
#include "util.h"
#include "acl.h"
#include <libgen.h>

extern t_pfs_domain *self_domain;
extern int pfs_up_log;
extern int start_rand;
static uint32_t pic_index = 0;

void create_push_rsp(t_pfs_tasklist *task, int fd)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->close = 1;
	char obuf[2048] = {0x0};
	t_task_base *base = &(task->task.base);
	char retbuf[256] = {0x0};
	snprintf(retbuf, sizeof(retbuf), "OK|%s|%s", base->retfile, self_domain->domain);
	int n = create_sig_msg(CMD_PUT_FILE_RSP, 0x0, (t_pfs_sig_body *)retbuf, obuf, sizeof(retbuf));
	set_client_data(fd, obuf, n);
	modify_fd_event(fd, EPOLLOUT);
}

void create_push_rsp_err(char *errmsg, int fd)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->close = 1;

	char obuf[2048] = {0x0};
	int n = create_sig_msg(CMD_PUT_FILE_RSP, 0x0, (t_pfs_sig_body *)errmsg, obuf, strlen(errmsg));
	set_client_data(fd, obuf, n);
	modify_fd_event(fd, EPOLLOUT);
}

int do_recv_task(int fd, t_pfs_sig_head *h, t_task_base *base)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	if (peer->sock_stat != PREPARE_RECVFILE)
	{
		LOG(pfs_up_log, LOG_ERROR, "fd[%d] status not recv [%x] file [%s]\n", fd, peer->sock_stat, base->filename);
		return RECV_CLOSE;
	}
	t_pfs_tasklist *task0 = peer->recvtask;
	t_task_base *base0 = &(task0->task.base);
	t_task_sub *sub0 = &(task0->task.sub);
	if (h->status != FILE_SYNC_DST_2_SRC)
	{
		LOG(pfs_up_log, LOG_ERROR, "fd[%d] status err file [%s][%x]\n", fd, base->filename, h->status);
		peer->sock_stat = IDLE;
		task0->task.base.overstatus = OVER_E_OPEN_SRCFILE;
		pfs_set_task(task0, TASK_Q_FIN);
		peer->recvtask = NULL;
		return RECV_CLOSE;
	}

	if (memcmp(base0->filemd5, base->filemd5, sizeof(base0->filemd5)))
	{
		memcpy(base0, base, sizeof(t_task_base));
		sub0->processlen = base->fsize;
	}
	if (peer->local_in_fd > 0)
		close(peer->local_in_fd);
	if (open_tmp_localfile_4_write(base0, &(peer->local_in_fd)) != LOCALFILE_OK) 
	{
		LOG(pfs_up_log, LOG_ERROR, "fd[%d] file [%s] open file err %m\n", fd, base->filename);
		if (peer->recvtask)
		{
			base0->overstatus = OVER_E_OPEN_DSTFILE;
			pfs_set_task(peer->recvtask, TASK_Q_CLEAN);
			peer->recvtask = NULL;
		}
		return RECV_CLOSE;
	}
	else
	{
		peer->sock_stat = RECVFILEING;
		LOG(pfs_up_log, LOG_NORMAL, "fd[%d] file [%s] prepare recv\n", fd, base->filename);
	}
	return RECV_ADD_EPOLLIN;
}

static void get_local_filename(t_task_base *base)
{
	if (base->type != TASK_ADDFILE)
		return;
	char localfile[256] = {0x0};
	char realfile[256] = {0x0};
	snprintf(realfile, sizeof(realfile), "%ld_%u_%04d", time(NULL), self_ip, pic_index++);
	base64_encode(realfile, strlen(realfile), localfile, 0);
	char *t = strchr(base->filename, '.');
	if (t)
		strcat(localfile, t);
	int d1 = 0, d2 = 0;
	get_max_free_storage(&d1, &d2);
	memset(base->filename, 0, sizeof(base->filename));
	snprintf(base->filename, sizeof(base->filename), "/%d/%d/%s", d1, d2, localfile);
	if (pic_index >= 99999)
		pic_index = 0;
}

inline void get_local_bname(t_task_base *base)
{
	int d1 = 0, d2 = 0;
	get_max_free_storage(&d1, &d2);
	char tmpname[128] = {0x0};
	snprintf(tmpname, sizeof(tmpname), "%s", basename(base->filename));
	int l = snprintf(base->filename, sizeof(base->filename), "/%d/%d/%s", d1, d2, tmpname);
	base->filename[l] = 0x0;
}

int do_push_task(int fd, t_pfs_sig_head *h, t_task_base *base)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->sock_stat = PREPARE_RECVFILE;
	t_pfs_tasklist *task = NULL;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_up_log, LOG_ERROR, "filename[%s] do_push_task ERROR!\n", base->filename);
		create_push_rsp_err("malloc error!", fd);
		return RECV_ADD_EPOLLOUT;
	}
	if (base->type != TASK_DELFILE)
	{
		if (h->status == 0)
			get_local_filename(base);
		else if (ADD_PATH_BY_BNAME == h->status)
			get_local_bname(base);
		else
			LOG(pfs_up_log, LOG_NORMAL, "filename[%s] do_push_task path by upload!\n", base->filename);
	}
	else
		LOG(pfs_up_log, LOG_NORMAL, "filename[%s] be delete!\n", base->filename);
	memset(&(task->task), 0, sizeof(task->task)); 
	t_task_sub *sub = &(task->task.sub);
	memset(sub, 0, sizeof(t_task_sub));
	ip2str(sub->peerip, peer->ip);
	sub->processlen = base->fsize;
	sub->starttime = time(NULL);
	sub->oper_type = OPER_PUT_REQ;
	sub->need_sync = TASK_SOURCE;

	t_task_base *base0 = &(task->task.base);
	memcpy(base0, base, sizeof(t_task_base));
	base0->starttime = time(NULL);
	peer->recvtask = task;
	if (base->type == TASK_DELFILE)
	{
		delete_localfile(base);
		base0->overstatus = OVER_OK;
		pfs_set_task(peer->recvtask, TASK_Q_FIN);
		peer->recvtask = NULL;
		create_push_rsp(task, fd);
		return RECV_SEND;
	}
	if (peer->local_in_fd > 0)
		close(peer->local_in_fd);
	if (ADD_PATH_BY_BNAME != h->status && check_localfile_md5(base0, VIDEOFILE) == LOCALFILE_OK)
	{
		LOG(pfs_up_log, LOG_NORMAL, "fd[%d] file [%s] md5 ok\n", fd, base0->filename);
		base0->overstatus = OVER_OK;
		pfs_set_task(peer->recvtask, TASK_Q_FIN);
		peer->recvtask = NULL;
		create_push_rsp_err("remote file exist", fd);
		return RECV_SEND;
	}
	if (open_tmp_localfile_4_write(base0, &(peer->local_in_fd)) != LOCALFILE_OK) 
	{
		LOG(pfs_up_log, LOG_ERROR, "fd[%d] file [%s] open file err %m\n", fd, base->filename);
		if (peer->recvtask)
		{
			base0->overstatus = OVER_E_OPEN_DSTFILE;
			pfs_set_task(peer->recvtask, TASK_Q_CLEAN);
			peer->recvtask = NULL;
		}
		create_push_rsp_err("open remote file err", fd);
		return RECV_ADD_EPOLLOUT;
	}
	else
	{
		peer->sock_stat = RECVFILEING;
		LOG(pfs_up_log, LOG_NORMAL, "fd[%d] file [%s]prepare recv\n", fd, base->filename);
	}
	return RECV_ADD_EPOLLIN;
}

