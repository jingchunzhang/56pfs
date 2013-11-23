/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "common.h"
#include "global.h"
#include "pfs_so.h"
#include "myepoll.h"
#include "protocol.h"
#include "pfs_data.h"
#include "util.h"
#include "acl.h"
#include "pfs_task.h"
#include "pfs_data_task.h"
#include "pfs_localfile.h"

int pfs_data_log = -1;
extern uint8_t self_stat ;
/* online list */
static list_head_t activelist;  //用来检测超时
static list_head_t online_list[256]; //用来快速定位查找

int svc_initconn(int fd); 
int active_send(pfs_cs_peer *peer, t_pfs_sig_head *h, t_pfs_sig_body *b);
const char *sock_stat_cmd[] = {"LOGOUT", "CONNECTED", "LOGIN", "IDLE", "PREPARE_RECVFILE", "RECVFILEING", "SENDFILEING", "LAST_STAT"};

#include "pfs_data_base.c"
#include "pfs_data_sub.c"

int svc_init() 
{
	char *logname = myconfig_get_value("log_data_logname");
	if (!logname)
		logname = "./data_log.log";

	char *cloglevel = myconfig_get_value("log_data_loglevel");
	int loglevel = LOG_NORMAL;
	if (cloglevel)
		loglevel = getloglevel(cloglevel);
	int logsize = myconfig_get_intval("log_data_logsize", 100);
	int logintval = myconfig_get_intval("log_data_logtime", 3600);
	int lognum = myconfig_get_intval("log_data_lognum", 10);
	pfs_data_log = registerlog(logname, loglevel, logsize, logintval, lognum);
	if (pfs_data_log < 0)
		return -1;
	LOG(pfs_data_log, LOG_NORMAL, "svc_init init log ok!\n");
	INIT_LIST_HEAD(&activelist);
	int i = 0;
	for (i = 0; i < 256; i++)
	{
		INIT_LIST_HEAD(&online_list[i]);
	}
	
	return 0;
}

int svc_pinit()
{
	return 0;
}

int svc_initconn(int fd) 
{
	uint32_t ip = getpeerip(fd);
	LOG(pfs_data_log, LOG_TRACE, "%s:%s:%d\n", ID, FUNC, LN);
	struct conn *curcon = &acon[fd];
	if (curcon->user == NULL)
		curcon->user = malloc(sizeof(pfs_cs_peer));
	if (curcon->user == NULL)
	{
		LOG(pfs_data_log, LOG_ERROR, "malloc err %m\n");
		char val[256] = {0x0};
		snprintf(val, sizeof(val), "malloc err %m");
		return RET_CLOSE_MALLOC;
	}
	pfs_cs_peer *peer;
	memset(curcon->user, 0, sizeof(pfs_cs_peer));
	peer = (pfs_cs_peer *)curcon->user;
	peer->hbtime = time(NULL);
	peer->sock_stat = CONNECTED;
	peer->fd = fd;
	peer->ip = ip;
	peer->mode = CON_PASSIVE;
	INIT_LIST_HEAD(&(peer->alist));
	INIT_LIST_HEAD(&(peer->hlist));
	list_move_tail(&(peer->alist), &activelist);
	list_add(&(peer->hlist), &online_list[ip&ALLMASK]);
	MD5Init(&(peer->ctx));
	LOG(pfs_data_log, LOG_TRACE, "a new fd[%d] init ok!\n", fd);
	return 0;
}

/*校验是否有一个完整请求*/
static int check_req(int fd)
{
	LOG(pfs_data_log, LOG_DEBUG, "%s:%s:%d\n", ID, FUNC, LN);
	t_pfs_sig_head h;
	t_pfs_sig_body b;
	char *data;
	size_t datalen;
	if (get_client_data(fd, &data, &datalen))
	{
		LOG(pfs_data_log, LOG_TRACE, "%s:%d fd[%d] no data!\n", FUNC, LN, fd);
		return -1;  /*no suffic data, need to get data more */
	}
	int ret = parse_sig_msg(&h, &b, data, datalen);
	if (ret > 0)
	{
		LOG(pfs_data_log, LOG_TRACE, "fd[%d] no suffic data!\n", fd);
		return -1;  /*no suffic data, need to get data more */
	}
	if (ret == E_PACKET_ERR_CLOSE)
	{
		LOG(pfs_data_log, LOG_ERROR, "fd[%d] ERROR PACKET bodylen is [%d], must be close now!\n", fd, h.bodylen);
		return RECV_CLOSE;
	}
	int clen = h.bodylen + SIG_HEADSIZE;
	ret = do_req(fd, &h, &b);
	consume_client_data(fd, clen);
	return ret;
}

int svc_recv(int fd) 
{
	char val[256] = {0x0};
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
recvfileing:
	peer->hbtime = time(NULL);
	list_move_tail(&(peer->alist), &activelist);
	LOG(pfs_data_log, LOG_TRACE, "fd[%d] sock stat %s!\n", fd, sock_stat_cmd[peer->sock_stat]);
	if (peer->sock_stat == RECVFILEING)
	{
		LOG(pfs_data_log, LOG_TRACE, "%s:%s:%d\n", ID, FUNC, LN);
		char *data;
		size_t datalen;
		t_pfs_tasklist *task0 = peer->recvtask;
		if(task0 == NULL)
		{
			LOG(pfs_data_log, LOG_ERROR, "recv task is null!\n");
			return RECV_CLOSE;  /* ERROR , close it */
		}
		t_pfs_taskinfo *task = &(peer->recvtask->task);
		if (get_client_data(fd, &data, &datalen))
		{
			LOG(pfs_data_log, LOG_TRACE, "%s:%d fd[%d] no data!\n", FUNC, LN, fd);
			return RECV_ADD_EPOLLIN;  /*no suffic data, need to get data more */
		}
		LOG(pfs_data_log, LOG_TRACE, "fd[%d] get file %s:%d!\n", fd, task->base.filename, datalen);
		int remainlen = task->sub.processlen - task->sub.lastlen;
		datalen = datalen <= remainlen ? datalen : remainlen ; 
		int n = write(peer->local_in_fd, data, datalen);
		if (n < 0)
		{
			LOG(pfs_data_log, LOG_ERROR, "fd[%d] write error %m close it!\n", fd);
			snprintf(val, sizeof(val), "write err %m");
			if (task0->task.sub.oper_type == OPER_PUT_REQ)
				create_push_rsp_err(val, fd);
			return RECV_ADD_EPOLLOUT;
		}
		consume_client_data(fd, n);
		task->sub.lastlen += n;
		MD5Update(&(peer->ctx),(const unsigned char*)data, n);
		if (task->sub.lastlen >= task->sub.processlen)
		{
			unsigned char omd5[16] = {0x0};
			MD5Final(omd5, &(peer->ctx));
			if (close_tmp_check_mv(&(task->base), peer->local_in_fd, omd5) != LOCALFILE_OK)
			{
				LOG(pfs_data_log, LOG_ERROR, "fd[%d] get file %s error!\n", fd, task->base.filename);
				task0->task.base.overstatus = OVER_E_MD5;
				peer->recvtask = NULL;
				pfs_set_task(task0, TASK_Q_FIN);
				snprintf(val, sizeof(val), "md5 error %m");
				if (task0->task.sub.oper_type == OPER_PUT_REQ)
					create_push_rsp_err(val, fd);
				return RECV_SEND;
			}
			else
			{
				LOG(pfs_data_log, LOG_NORMAL, "fd[%d:%u] get file %s ok!\n", fd, peer->ip, task->base.filename);
				task0->task.base.overstatus = OVER_OK;
				if (task0->task.sub.oper_type == OPER_PUT_REQ)
					create_push_rsp(task0, fd);
				pfs_set_task(task0, TASK_Q_FIN);
			}
			peer->local_in_fd = -1;
			peer->recvtask = NULL;
			peer->sock_stat = IDLE;
			return RECV_SEND;
		}
		else
			return RECV_ADD_EPOLLIN;  /*no suffic data, need to get data more */
	}
	
	int ret = RECV_ADD_EPOLLIN;;
	int subret = 0;
	while (1)
	{
		subret = check_req(fd);
		if (subret == -1)
			break;
		if (subret == RECV_ADD_EPOLLOUT)
			return RECV_ADD_EPOLLOUT;
		if (subret == RECV_SEND)
			return RECV_SEND;
		if (subret == RECV_CLOSE)
			return RECV_CLOSE;

		if (peer->sock_stat == RECVFILEING)
			goto recvfileing;
		if (peer->sock_stat == SENDFILEING)
			return RECV_ADD_EPOLLOUT;
	}
	return ret;
}

int svc_send_once(int fd)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->hbtime = time(NULL);
	return SEND_ADD_EPOLLIN;
}

int svc_send(int fd)
{
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->hbtime = time(NULL);
	list_move_tail(&(peer->alist), &activelist);
	if (peer->sock_stat == SENDFILEING)
	{
		LOG(pfs_data_log, LOG_DEBUG, "%s:%s:%d\n", ID, FUNC, LN);
		t_pfs_tasklist *tasklist = peer->sendtask;
		t_pfs_taskinfo *task = &(peer->sendtask->task);
		if (curcon->send_len >= peer->headlen + task->sub.processlen)
		{
			LOG(pfs_data_log, LOG_DEBUG, "fd[%d:%u] send file %s ok!\n", fd, peer->ip, task->base.filename);
			peer->sock_stat = IDLE;
			task->base.overstatus = OVER_OK;
			pfs_set_task(tasklist, TASK_Q_FIN);
			peer->sendtask = NULL;
		}
		else
		{
			LOG(pfs_data_log, LOG_ERROR, "fd[%d:%u] send file %s error [%d:%d:%d]!\n", fd, peer->ip, task->base.filename, curcon->send_len, peer->headlen, task->sub.processlen);
			task->base.overstatus = OVER_SEND_LEN;
			peer->sock_stat = IDLE;
			pfs_set_task(tasklist, TASK_Q_FIN);
			peer->sendtask = NULL;
			return SEND_CLOSE;
		}
	}
	if (peer->close)
		return SEND_CLOSE;
	return SEND_ADD_EPOLLIN;
}

void svc_timeout()
{
	time_t now = time(NULL);
	int to = g_config.timeout * 10;
	pfs_cs_peer *peer = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(peer, l, &activelist, alist)
	{
		if (peer == NULL)
			continue;   /*bugs */
		if (now - peer->hbtime < to)
			break;
		LOG(pfs_data_log, LOG_DEBUG, "timeout close %d [%lu:%lu] %d\n", peer->fd, now, peer->hbtime, peer->role);
		do_close(peer->fd);
	}
	check_task();
}

void svc_finiconn(int fd)
{
	LOG(pfs_data_log, LOG_TRACE, "close %d\n", fd);
	struct conn *curcon = &acon[fd];
	if (curcon->user == NULL)
		return;
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	if (peer->local_in_fd > 0)
		close(peer->local_in_fd);
	list_del_init(&(peer->alist));
	list_del_init(&(peer->hlist));
	t_pfs_tasklist *tasklist = NULL;
	if (peer->sendtask)
	{
		tasklist = peer->sendtask;
		LOG(pfs_data_log, LOG_ERROR, "abort send %s!\n", tasklist->task.base.filename);
		tasklist->task.base.overstatus = OVER_SEND_LEN;
		pfs_set_task(tasklist, TASK_Q_FIN);
	}
	if (peer->recvtask)
	{
		tasklist = peer->recvtask;
		LOG(pfs_data_log, LOG_ERROR, "re execute %s!\n", tasklist->task.base.filename);
		real_rm_file(tasklist->task.base.tmpfile);
		tasklist->task.base.retry++;
		pfs_set_task(tasklist, TASK_Q_WAIT);
	}
	memset(curcon->user, 0, sizeof(pfs_cs_peer));
	free(curcon->user);
	curcon->user = NULL;
}
