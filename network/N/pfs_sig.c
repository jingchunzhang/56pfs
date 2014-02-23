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
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "common.h"
#include "global.h"
#include "pfs_so.h"
#include "myepoll.h"
#include "protocol.h"
#include "pfs_sig.h"
#include "pfs_task.h"
#include "pfs_del_file.h"
#include "util.h"
#include "acl.h"

int pfs_sig_log = -1;
extern uint8_t self_stat ;
extern t_g_config g_config;

/* online list */
static list_head_t activelist;  //用来检测超时
static list_head_t online_list[256]; //用来快速定位查找

static list_head_t domain_change_list[16]; 
static list_head_t storage_select_list[16]; 

uint32_t proxy_ip[16];
char proxy_sip[16][16];
int svc_initconn(int fd); 
int active_send(pfs_fcs_peer *peer, t_pfs_sig_head *h, t_pfs_sig_body *b);
#include "pfs_sig_base.c"
#include "pfs_sig_sub.c"

int svc_init()  
{
	char *logname = myconfig_get_value("log_sig_logname");
	if (!logname)
		logname = "./sig_log.log";

	char *cloglevel = myconfig_get_value("log_sig_loglevel");
	int loglevel = LOG_NORMAL;
	if (cloglevel)
		loglevel = getloglevel(cloglevel);

	int logsize = myconfig_get_intval("log_sig_logsize", 100);
	int logintval = myconfig_get_intval("log_sig_logtime", 3600);
	int lognum = myconfig_get_intval("log_sig_lognum", 10);

	pfs_sig_log = registerlog(logname, loglevel, logsize, logintval, lognum);
	if (pfs_sig_log < 0)
		return -1;
	LOG(pfs_sig_log, LOG_DEBUG, "svc_init init log ok!\n");

	INIT_LIST_HEAD(&activelist);
	int i = 0;
	for (i = 0; i < 256; i++)
	{
		INIT_LIST_HEAD(&online_list[i]);
	}

	for (i = 0; i < 16; i++)
	{
		INIT_LIST_HEAD(&domain_change_list[i]);
		INIT_LIST_HEAD(&storage_select_list[i]);
	}

	char *name_change_file = myconfig_get_value("domain_change_file");
	if (name_change_file)
	{
		if (do_init_name_change(name_change_file))
		{
			LOG(pfs_sig_log, LOG_ERROR, "do_init_name_change %s err %m\n", name_change_file);
			return -1;
		}
	}

	char *storage_select_file = myconfig_get_value("storage_select_file");
	if (storage_select_file)
	{
		if (do_init_storage_select(storage_select_file))
		{
			LOG(pfs_sig_log, LOG_ERROR, "do_init_storage_select %s err %m\n", storage_select_file);
			return -1;
		}
	}
	memset(proxy_ip, 0, sizeof(proxy_ip));
	char *proxy_file = myconfig_get_value("proxy_file");
	if (proxy_file)
	{
		if (do_init_proxy(proxy_file))
		{
			LOG(pfs_sig_log, LOG_ERROR, "do_init_proxy %s err %m\n", proxy_file);
			return -1;
		}
	}

	time_t now = time(NULL);
	update_sort_policy(now);
	self_stat = ON_LINE;
	return 0;
}

int svc_pinit()
{
	return 0;
}

int svc_initconn(int fd) 
{
	LOG(pfs_sig_log, LOG_DEBUG, "%s:%s:%d\n", ID, FUNC, LN);
	uint32_t ip = getpeerip(fd);
	pfs_fcs_peer *peer = NULL;
	struct conn *curcon = &acon[fd];
	if (curcon->user == NULL)
		curcon->user = malloc(sizeof(pfs_fcs_peer));
	if (curcon->user == NULL)
	{
		LOG(pfs_sig_log, LOG_ERROR, "malloc err %m\n");
		return RET_CLOSE_MALLOC;
	}
	memset(curcon->user, 0, sizeof(pfs_fcs_peer));
	peer = (pfs_fcs_peer *)curcon->user;
	peer->hbtime = time(NULL);
	peer->fd = fd;
	peer->ip = ip;
	INIT_LIST_HEAD(&(peer->alist));
	INIT_LIST_HEAD(&(peer->hlist));
	list_move_tail(&(peer->alist), &activelist);
	list_add(&(peer->hlist), &online_list[ip&ALLMASK]);
	LOG(pfs_sig_log, LOG_DEBUG, "a new fd[%d] init ok!\n", fd);
	return 0;
}

/*校验是否有一个完整请求*/
static int check_req(int fd)
{
	t_pfs_sig_head h;
	t_pfs_sig_body b;
	memset(&h, 0, sizeof(h));
	memset(&b, 0, sizeof(b));
	char *data;
	size_t datalen;
	if (get_client_data(fd, &data, &datalen))
	{
		LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] no data!\n", fd);
		return -1;  /*no suffic data, need to get data more */
	}
	int ret = parse_sig_msg(&h, &b, data, datalen);
	if(ret > 0)
	{
		LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] no suffic data!\n", fd);
		return -1;  /*no suffic data, need to get data more */
	}
	if (ret == E_PACKET_ERR_CLOSE)	
	{		
		LOG(pfs_sig_log, LOG_ERROR, "fd[%d] ERROR PACKET bodylen is [%d], must be close now!\n", fd, h.bodylen);
		return RECV_CLOSE;	
	}
	int clen = h.bodylen + SIG_HEADSIZE;
	ret = do_req(fd, &h, &b);
	consume_client_data(fd, clen);
	return ret;
}

int svc_recv(int fd) 
{
	struct conn *curcon = &acon[fd];
	pfs_fcs_peer *peer = (pfs_fcs_peer *) curcon->user;
	peer->hbtime = time(NULL);
	list_move_tail(&(peer->alist), &activelist);
	
	int ret = RECV_ADD_EPOLLIN;;
	int subret = 0;
	while (1)
	{
		subret = check_req(fd);
		if (subret == -1)
			break;
		if (subret == RECV_CLOSE)
			return RECV_CLOSE;
		ret |= subret;
	}
	return ret;
}

int svc_send(int fd)
{
	struct conn *curcon = &acon[fd];
	pfs_fcs_peer *peer = (pfs_fcs_peer *) curcon->user;
	if (peer->close)
		return SEND_CLOSE;
	peer->hbtime = time(NULL);
	list_move_tail(&(peer->alist), &activelist);
	return SEND_ADD_EPOLLIN;
}

void svc_timeout()
{
	time_t now = time(NULL);
	int to = g_config.timeout * 10;

	pfs_fcs_peer *peer = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(peer, l, &activelist, alist)
	{
		if (now - peer->hbtime < g_config.timeout)
			continue;
		if (now - peer->hbtime > to)
		{
			LOG(pfs_sig_log, LOG_DEBUG, "timeout close %d [%lu:%lu]\n", peer->fd, now, peer->hbtime);
			do_close(peer->fd);
		}
	}

	update_sort_policy(now);
}

void svc_finiconn(int fd)
{
	LOG(pfs_sig_log, LOG_DEBUG, "close %d\n", fd);
	struct conn *curcon = &acon[fd];
	if (curcon->user == NULL)
		return;
	pfs_fcs_peer *peer = (pfs_fcs_peer *) curcon->user;
	list_del_init(&(peer->alist));
	list_del_init(&(peer->hlist));
	memset(curcon->user, 0, sizeof(pfs_fcs_peer));
	free(curcon->user);
	curcon->user = NULL;
}
