/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_sig.h"
#include "common.h"
#include "global.h"
#include "pfs_so.h"
#include "myepoll.h"
#include "protocol.h"
#include "pfs_localfile.h"
#include "pfs_del_file.h"
#include "pfs_tmp_status.h"
#include "pfs_sig.h"
#include "util.h"
#include "acl.h"

extern char hostname[64];
extern int pfs_sig_log;
extern int pfs_sig_log_err;
extern t_sync_para sync_para;
extern t_pfs_domain *self_domain;

int do_newtask(int fd, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	if (self_stat == OFF_LINE)
		return 0;
	t_task_base *base = (t_task_base*) b;
	base->starttime = time(NULL);
	base->dstip = self_ip;
	dump_task_info (pfs_sig_log, (char *) FUNC, LN, (t_task_base *)b);
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;

	t_task_sub sub;
	memset(&sub, 0, sizeof(sub));
	sub.processlen = base->fsize;
	sub.starttime = time(NULL);
	sub.lasttime = sub.starttime;
	sub.oper_type = OPER_GET_REQ;

	t_pfs_tasklist *task = NULL;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] do_newtask ERROR!\n", fd);
		h->status = TASK_FAILED;
		return -1;
	}

	LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] upip is [%u] filename [%s] role [%d]\n", fd, peer->ip, base->filename, peer->role);
	ip2str(sub.peerip, peer->ip);

	memset(&(task->task), 0, sizeof(task->task));
	memcpy(&(task->task.base), base, sizeof(t_task_base));
	memcpy(&(task->task.sub), &sub, sizeof(t_task_sub));

	dump_task_info (pfs_sig_log, (char *) FUNC, LN, &(task->task.base));
	if (base->type == TASK_DELFILE)
	{
		if (delete_localfile(base))
			LOG(pfs_sig_log_err, LOG_ERROR, "%s unlink %m!\n", base->filename);
		else
			LOG(pfs_sig_log, LOG_DEBUG, "%s unlink ok!\n", base->filename);
		task->task.base.overstatus = OVER_OK;
		pfs_set_task(task, TASK_Q_FIN);  
		return 0;
	}
	if (check_localfile_md5(base, VIDEOFILE) == LOCALFILE_OK)
	{
		LOG(pfs_sig_log, LOG_DEBUG, "%s check md5 ok!\n", base->filename);
		task->task.base.overstatus = OVER_OK;
		pfs_set_task(task, TASK_Q_FIN);  
		return 0;
	}
	task->task.base.overstatus = OVER_UNKNOWN;
	if(h->status != TASK_Q_SYNC_DIR)
	{
		pfs_set_task(task, TASK_Q_WAIT);
		set_task_to_tmp(task);
	}
	else
		pfs_set_task(task, TASK_Q_SYNC_DIR);
	return 0;
}
		
int do_dispatch_task(char *sip, t_pfs_tasklist *task, pfs_cs_peer *peer, uint8_t status)
{
	t_task_base *base = &(task->task.base);
	LOG(pfs_sig_log, LOG_NORMAL, "%s:%s:%d  dispatch [%s][%c] to [%s]!\n", ID, FUNC, LN, base->filename, base->type, sip);
	char obuf[2048] = {0x0};
	int n = create_sig_msg(NEWTASK_REQ, status, (t_pfs_sig_body *)base, obuf, sizeof(t_task_base));
	set_client_data(peer->fd, obuf, n);
	modify_fd_event(peer->fd, EPOLLOUT);
	return 0;
}
		
static void create_delay_task(uint32_t ip, t_task_base *base)
{
	t_pfs_tasklist *task = NULL;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "[%s] do_newtask ERROR!\n", FUNC);
		return;
	}
	list_del_init(&(task->userlist));
	base->starttime = time(NULL);
	memset(&(task->task), 0, sizeof(task->task));
	memcpy(&(task->task.base), base, sizeof(t_task_base));

	ip2str(task->task.sub.peerip, ip);
	task->task.sub.oper_type = OPER_SYNC_2_DOMAIN;
	pfs_set_task(task, TASK_Q_WAIT_SYNC);
	set_task_to_tmp(task);
}

int sync_task_2_domain(t_pfs_tasklist *task)
{
	pfs_cs_peer *peer;
	int i = 0;
	for (i = 0; i < self_domain->ipcount; i++)
	{
		if (self_domain->ips[i] == self_ip)
			continue;
		if (self_domain->offline[i])
			continue;
		if (find_ip_stat(self_domain->ips[i], &peer))
		{
			LOG(pfs_sig_log_err, LOG_ERROR, "ip[%s] not in active !\n", self_domain->sips[i]);
			create_delay_task(self_domain->ips[i], &(task->task.base));
			continue;
		}
		do_dispatch_task(self_domain->sips[i], task, peer, TASK_SYNC);
	}
	return 0;
}

void do_sync_dir_req(int fd, t_pfs_sig_body *b)
{
	t_pfs_tasklist *task = NULL;
	char obuf[2048] = {0x0};
	int n = 0;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] do_sync_dir_req ERROR!\n", fd);
		memset(obuf, 0, sizeof(obuf));
		n = create_sig_msg(SYNC_DIR_RSP, TASK_SYNC_DIR, b, obuf, sizeof(t_pfs_sync_task));
		set_client_data(fd, obuf, n);
		modify_fd_event(fd, EPOLLOUT);
		return;
	}
	t_task_base *base = &(task->task.base);
	memset(base, 0, sizeof(t_task_base));
	memcpy(base, b, sizeof(t_pfs_sync_task));
	base->dstip = getpeerip(fd);
	pfs_set_task(task, TASK_Q_SYNC_DIR_REQ);
}

void do_sync_del_req(int fd, t_pfs_sig_body *b)
{
	t_pfs_sync_task * task = (t_pfs_sync_task *)b;
	char obuf[2048] = {0x0};
	int n = 0;
	t_task_base base;
	memset(&base, 0, sizeof(base));
	base.type = TASK_DELFILE;
	int next = find_last_index(&base, task->starttime);
	if (next == -1)
		LOG(glogfd, LOG_NORMAL, "no more del file [%s]\n", ctime(&(task->starttime)));
	else
	{
		time_t cur = time(NULL);
		while(1) 
		{
			memset(obuf, 0, sizeof(obuf));
			n = create_sig_msg(NEWTASK_REQ, TASK_SYNC_DIR, (t_pfs_sig_body *)&base, obuf, sizeof(t_task_base));
			set_client_data(fd, obuf, n);
			next++;
			memset(base.filename, 0, sizeof(base.filename));
			next = get_from_del_file (&base, next, cur);
			if (next == -1)
				break;
		}
	}
	memset(obuf, 0, sizeof(obuf));
	n = create_sig_msg(SYNC_DEL_RSP, TASK_SYNC_DIR, b, obuf, sizeof(t_pfs_sync_task));
	set_client_data(fd, obuf, n);
	modify_fd_event(fd, EPOLLOUT);
}

