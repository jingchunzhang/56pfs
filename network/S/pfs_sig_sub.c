/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "c_api.h"
extern uint32_t maxfreevalue;  /*Gbytes*/

static void do_fin_task()
{
	while (1)
	{
		t_pfs_tasklist *task = NULL;
		if (pfs_get_task(&task, TASK_Q_FIN))
			return ;
		dump_task_info (pfs_sig_log, (char*)FUNC, LN, &(task->task.base));
		if (OVER_E_MD5 == task->task.base.overstatus)
		{
			if (g_config.retry && task->task.base.retry < g_config.retry)
			{
				task->task.base.retry++;
				task->task.base.overstatus = OVER_UNKNOWN;
				LOG(pfs_sig_log, LOG_NORMAL, "retry[%d:%d:%s]\n", task->task.base.retry, g_config.retry, task->task.base.filename);
				pfs_set_task(task, TASK_Q_WAIT);
				continue;
			}
		}
		if (task->task.sub.need_sync == TASK_SOURCE)
		{
			if (task->task.base.overstatus == OVER_OK || task->task.base.overstatus == OVER_UNLINK)
				if (sync_task_2_domain(task))
				{
					pfs_set_task(task, TASK_Q_DELAY);
					continue;
				}
		}
		else if (task->task.sub.need_sync == TASK_SYNC_ISDIR)
		{
			sync_para.total_synced_task++;
			if (sync_para.flag == 2 && sync_para.total_synced_task >= sync_para.total_sync_task)
			{
				LOG(pfs_sig_log, LOG_NORMAL, "SYNC[%d:%d]\n", sync_para.total_synced_task, sync_para.total_sync_task);
				if (self_stat != OFF_LINE)
					self_stat = ON_LINE;
			}
		}
		if ((task->task.base.type == TASK_ADDFILE) && task->task.base.overstatus == OVER_OK)
			set_time_stamp(&(task->task.base));
		if (task->task.user)
		{
			LOG(pfs_sig_log, LOG_DEBUG, "%s:%d:%d\n", ID, LN, task->task.sub.oper_type);
			t_tmp_status *tmp = task->task.user;
			set_tmp_blank(tmp->pos, tmp);
			task->task.user = NULL;
		}
		pfs_set_task(task, TASK_Q_CLEAN);
	}
}

static void do_check_sync_task()
{
	t_pfs_tasklist *task = NULL;
	int ret = 0;
	while (1)
	{
		ret = pfs_get_task(&task, TASK_Q_SYNC_DIR_RSP);
		if (ret != GET_TASK_Q_OK)
			break;
		pfs_cs_peer *peer;
		if (find_ip_stat(task->task.base.dstip, &peer))
		{
			LOG(pfs_sig_log, LOG_ERROR, "find_ip_stat TASK_Q_SYNC_DIR_RSP error %u\n", task->task.base.dstip);
			pfs_set_task(task, TASK_Q_HOME);
			continue;
		}
		char obuf[2048] = {0x0};
		int n = 0;
		n = create_sig_msg(SYNC_DIR_RSP, TASK_SYNC_DIR, (t_pfs_sig_body *)(&(task->task.base)), obuf, sizeof(t_pfs_sync_task));
		set_client_data(peer->fd, obuf, n);
		modify_fd_event(peer->fd, EPOLLOUT);
		pfs_set_task(task, TASK_Q_HOME);
	}
}

static void check_task()
{
	t_pfs_tasklist *task = NULL;
	int ret = 0;
	int once = 0;
	while (1)
	{
		if (once >= g_config.max_task_run_once)
		{
			LOG(pfs_sig_log, LOG_DEBUG, "too many task in cs %d %d\n", once, g_config.max_task_run_once);
			break;
		}
		ret = pfs_get_task(&task, TASK_Q_WAIT_SYNC_IP);
		if (ret != GET_TASK_Q_OK)
		{
			LOG(pfs_sig_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
			break;
		}
		once++;
		pfs_set_task(task, TASK_Q_WAIT_SYNC);
	}

	once = 0;
	while (1)
	{
		if (once >= g_config.max_task_run_once)
		{
			LOG(pfs_sig_log, LOG_DEBUG, "too many task in cs %d %d\n", once, g_config.max_task_run_once);
			break;
		}
		ret = pfs_get_task(&task, TASK_Q_WAIT_SYNC_DIR_TMP);
		if (ret != GET_TASK_Q_OK)
		{
			LOG(pfs_sig_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
			break;
		}
		once++;
		pfs_set_task(task, TASK_Q_WAIT_SYNC_DIR);
	}

	once = 0;
	while (1)
	{
		if (once >= g_config.max_task_run_once)
		{
			LOG(pfs_sig_log, LOG_DEBUG, "too many task in cs %d %d\n", once, g_config.max_task_run_once);
			break;
		}
		uint8_t status = TASK_SYNC;
		ret = pfs_get_task(&task, TASK_Q_WAIT_SYNC);
		if (ret != GET_TASK_Q_OK)
		{
			ret = pfs_get_task(&task, TASK_Q_WAIT_SYNC_DIR);
			if (ret != GET_TASK_Q_OK)
			{
				LOG(pfs_sig_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
				break;
			}
			status = TASK_Q_SYNC_DIR;
		}
		once++;
		uint32_t ip = str2ip(task->task.sub.peerip);
		pfs_cs_peer *peer = NULL;
		if (find_ip_stat(ip, &peer))
		{
			LOG(pfs_sig_log_err, LOG_ERROR, "ip[%u] not in active !\n", ip);
			if (status == TASK_Q_SYNC_DIR)
				pfs_set_task(task, TASK_Q_WAIT_SYNC_DIR_TMP);
			else
				pfs_set_task(task, TASK_Q_WAIT_SYNC_IP);
			continue;
		}
		char sip[16] = {0x0};
		ip2str(sip, ip);
		do_dispatch_task(sip, task, peer, status);
		if (task->task.user)
		{
			t_tmp_status *tmp = task->task.user;
			set_tmp_blank(tmp->pos, tmp);
			task->task.user = NULL;
		}
		pfs_set_task(task, TASK_Q_HOME);  /*同步信令 不作为任务上报*/
	}
	do_fin_task();
	do_check_sync_task();
}

int active_send(pfs_cs_peer *peer, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	LOG(pfs_sig_log, LOG_DEBUG, "send %d cmdid %x\n", peer->fd, h->cmdid);
	char obuf[2048] = {0x0};
	size_t n = 0;
	peer->hbtime = time(NULL);
	n = create_sig_msg(h->cmdid, h->status, b, obuf, h->bodylen);
	set_client_data(peer->fd, obuf, n);
	modify_fd_event(peer->fd, EPOLLOUT);
	return 0;
}

static void sync_diskerr_2_domain()
{
	uint8_t *errdisk;
	if (get_self_err_disk(&errdisk))
	{
		LOG(pfs_sig_log, LOG_ERROR, "get_self_err_disk ERR!\n");
		return;
	}
	t_pfs_sig_head h;
	t_pfs_sig_body b;

	h.bodylen = MAXDISKS;
	memcpy(b.body, errdisk, h.bodylen);
	h.cmdid = DISKINFO_RSP;
	h.status = HB_C_2_C;

	pfs_cs_peer *peer;
	int i;
	for (i = 0; i < self_domain->ipcount; i++)
	{
		if (self_domain->ips[i] == self_ip)
			continue;
		if (self_domain->offline[i])
			continue;
		if (find_ip_stat(self_domain->ips[i], &peer))
		{
			LOG(pfs_sig_log_err, LOG_ERROR, "ip[%s] not in active !\n", self_domain->sips[i]);
			continue;
		}
		active_send(peer, &h, &b);
	}
}

void report_local_diskerr()
{
	LOG(pfs_sig_log, LOG_NORMAL, "start to %s\n", __FUNCTION__);
	sync_diskerr_2_domain();
	t_pfs_sig_head h1;
	t_pfs_sig_body b1;

	uint32_t load = getloadinfo();
	uint32_t taskcount = get_task_count(TASK_Q_WAIT);
	h1.bodylen = sizeof(uint32_t) * 3;
	memcpy(b1.body, &load, sizeof(uint32_t));
	memcpy(b1.body + sizeof(uint32_t), &taskcount, sizeof(uint32_t));
	memcpy(b1.body + sizeof(uint32_t)*2, &maxfreevalue, sizeof(uint32_t));
	h1.cmdid = LOADINFO_REQ;
	h1.status = HB_C_2_C;
	pfs_cs_peer *peer = NULL;
	int i = 0;
	for (i = 0; i < MAX_NAMESERVER; i++)
	{
		if (pfs_nameserver[i].uip == 0)
			return;
		if (find_ip_stat(pfs_nameserver[i].uip, &peer) == 0)
		{
			active_send(peer, &h1, &b1);
		}
		else
			LOG(pfs_sig_log, LOG_ERROR, "nameserver %s not online\n", pfs_nameserver[i].ip);
	}
}

static int do_req(int fd, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	struct conn *curcon = &acon[fd];
	char obuf[2048] = {0x0};
	size_t n = 0;
	t_pfs_sig_body ob;
	memset(&ob, 0, sizeof(ob));
	int bodylen = 0;
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->hbtime = time(NULL);
	bodylen = sizeof(self_stat);
	memcpy(ob.body, &self_stat, sizeof(self_stat));
	switch(h->cmdid)
	{
		case HEARTBEAT_REQ:
			n = create_sig_msg(HEARTBEAT_RSP, h->status, &ob, obuf, bodylen);
			set_client_data(fd, obuf, n);
			peer->sock_stat = SEND_LAST;
			if (h->bodylen != 1)
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad hb , no SERVER_STAT\n", fd);
			else
				peer->server_stat = *(b->body);
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a HB\n", fd);
			return RECV_ADD_EPOLLOUT;

		case HEARTBEAT_RSP:
			peer->sock_stat = IDLE;
			if (h->bodylen != 1)
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad hb rsp, no SERVER_STAT\n", fd);
			else
				peer->server_stat = *(b->body);
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a HB RSP\n", fd);
			return RECV_ADD_EPOLLIN;

		case NEWTASK_REQ:
			if (h->bodylen != sizeof(t_task_base))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad NEWTASK_Q_REQ bodylen[%d]!\n", fd, h->bodylen);
				return RECV_CLOSE;
			}
			if (do_newtask(fd, h, b))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] do_newtask error!\n", fd);
				n = create_sig_msg(NEWTASK_RSP, h->status, b, obuf, sizeof(t_task_base));
				set_client_data(fd, obuf, n);
				peer->sock_stat = SEND_LAST;
				return RECV_ADD_EPOLLOUT;
			}
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a NEWTASK_Q_REQ\n", fd);
			return RECV_ADD_EPOLLIN;

		case NEWTASK_RSP:
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a NEWTASK_Q_RSP\n", fd);
			return RECV_ADD_EPOLLIN;

		case SYNC_DIR_REQ:
			if (h->bodylen != sizeof(t_pfs_sync_task))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad SYNC_DIR_REQ bodylen[%d]!\n", fd, h->bodylen);
				return RECV_ADD_EPOLLIN;
			}
			LOG(pfs_sig_log, LOG_NORMAL, "fd[%d] recv a SYNC_DIR_REQ\n", fd);
			do_sync_dir_req(fd, b);
			return RECV_ADD_EPOLLOUT;

		case SYNC_DIR_RSP:
			LOG(pfs_sig_log, LOG_NORMAL, "fd[%d] recv a SYNC_DIR_RSP\n", fd);
			if (peer->pfs_sync_list)
			{
				free(peer->pfs_sync_list);
				peer->pfs_sync_list = NULL;
			}
			sync_para.flag = 0;
			return RECV_ADD_EPOLLIN;

		case SYNC_DEL_REQ:
			if (h->bodylen != sizeof(t_pfs_sync_task))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad SYNC_DEL_REQ bodylen[%d]!\n", fd, h->bodylen);
				return RECV_ADD_EPOLLIN;
			}
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a SYNC_DEL_REQ\n", fd);
			do_sync_del_req(fd, b);
			return RECV_ADD_EPOLLOUT;

		case SYNC_DEL_RSP:
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a SYNC_DEL_RSP\n", fd);
			if (peer->pfs_sync_list)
			{
				free(peer->pfs_sync_list);
				peer->pfs_sync_list = NULL;
			}
			sync_para.flag = 0;
			return RECV_ADD_EPOLLIN;

		case DISKINFO_RSP:
			LOG(pfs_sig_log, LOG_NORMAL, "fd[%d] recv a DISKINFO_RSP\n", fd);
			if (h->bodylen != MAXDISKS)
				LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad DISKINFO_RSP bodylen[%d]!\n", fd, h->bodylen);
			else
				set_domain_diskerr(b->body, getpeerip(fd));
			break;

		default:
			LOG(pfs_sig_log_err, LOG_ERROR, "fd[%d] recv a bad cmd [%x] status[%x]!\n", fd, h->cmdid, h->status);
			return RECV_CLOSE;
	}
	return RECV_ADD_EPOLLIN;
}
