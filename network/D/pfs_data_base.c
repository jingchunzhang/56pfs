/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
/*
 *base文件，查询基础配置信息，设置相关基础状态及其它```
 *Tracker 数目较少，放在一个静态数组
 *CS和FCS数目较多，放在hash链表
 *CS FCS ip信息采用uint32_t 存储，便于存储和查找
 */
volatile extern int maintain ;		//1-维护配置 0 -可以使用
extern t_pfs_up_proxy g_proxy;

static int get_ip_connect_count(uint32_t ip)
{
	int count = 0;
	list_head_t *hashlist = &(online_list[ALLMASK&ip]);
	pfs_cs_peer *peer = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(peer, l, hashlist, hlist)
	{
		if (peer->ip == ip && peer->mode == CON_ACTIVE)
		{
			count++;
		}
	}
	return count;
}

static void active_connect(char *ip)
{
	int port = g_config.data_port;
	int fd = createsocket(ip, port);
	if (fd < 0)
	{
		LOG(pfs_data_log, LOG_ERROR, "connect %s:%d err %m\n", ip, port);
		char val[256] = {0x0};
		snprintf(val, sizeof(val), "connect %s:%d err %m\n", ip, port);
		SetStr(PFS_STR_CONNECT_E, val);
		return;
	}
	if (svc_initconn(fd))
	{
		LOG(pfs_data_log, LOG_ERROR, "svc_initconn err %m\n");
		close(fd);
		return;
	}
	add_fd_2_efd(fd);
	LOG(pfs_data_log, LOG_NORMAL, "connect %s:%d\n", ip, port);
	struct conn *curcon = &acon[fd];
	pfs_cs_peer *peer = (pfs_cs_peer *) curcon->user;
	peer->sock_stat = IDLE;
	peer->mode = CON_ACTIVE;
}

/*find 活动链接信息 */
int find_ip_stat(uint32_t ip, pfs_cs_peer **dpeer, int mode, int status)
{
	LOG(pfs_data_log, LOG_DEBUG, "%s %d\n", FUNC, LN);
	list_head_t *hashlist = &(online_list[ALLMASK&ip]);
	pfs_cs_peer *peer = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(peer, l, hashlist, hlist)
	{
		if (peer->ip == ip)
		{
			if (mode == peer->mode && status == peer->sock_stat)
			{
				*dpeer = peer;
				LOG(pfs_data_log, LOG_DEBUG, "%s %d\n", FUNC, LN);
				return 0;
			}
		}
	}
	LOG(pfs_data_log, LOG_DEBUG, "%s %d\n", FUNC, LN);
	return -1;
}

void check_task()
{
	t_pfs_tasklist *task = NULL;
	int ret = 0;
	while (1)
	{
		ret = pfs_get_task(&task, TASK_Q_WAIT_TMP);
		if (ret != GET_TASK_Q_OK)
		{
			LOG(pfs_data_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
			break;
		}
		pfs_set_task(task, TASK_Q_WAIT);
	}

	while (1)
	{
		ret = pfs_get_task(&task, TASK_Q_SYNC_DIR_TMP);
		if (ret != GET_TASK_Q_OK)
		{
			LOG(pfs_data_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
			break;
		}
		pfs_set_task(task, TASK_Q_SYNC_DIR);
	}

	uint16_t once_run = 0;
	while (1)
	{
		once_run++;
		if (once_run >= g_config.max_task_run_once)
			return;
		ret = pfs_get_task(&task, TASK_Q_WAIT);
		if (ret != GET_TASK_Q_OK)
		{
			LOG(pfs_data_log, LOG_TRACE, "pfs_get_task get notihng %d\n", ret);
			ret = pfs_get_task(&task, TASK_Q_SYNC_DIR);
			if (ret != GET_TASK_Q_OK)
				return ;
			else
				LOG(pfs_data_log, LOG_DEBUG, "Process TASK_Q_SYNC_DIR!\n");
		}
		t_task_sub *sub = &(task->task.sub);
		t_task_base *base = &(task->task.base);
		LOG(pfs_data_log, LOG_DEBUG, "%s be get from wait queue!\n", base->filename);
		if (base->retry > g_config.retry)
		{
			LOG(pfs_data_log, LOG_DEBUG, "%s too many try!\n", base->filename, base->retry, g_config.retry);
			base->overstatus = OVER_TOO_MANY_TRY;
			pfs_set_task(task, TASK_Q_FIN);  
			continue;
		}
		if (base->fsize > 0 && check_localfile_md5(base, VIDEOFILE) == LOCALFILE_OK)
		{
			LOG(pfs_data_log, LOG_DEBUG, "%s check md5 ok!\n", base->filename);
			base->overstatus = OVER_OK;
			pfs_set_task(task, TASK_Q_FIN);  
			continue;
		}
		pfs_cs_peer *peer = NULL;
		find_ip_stat(str2ip(sub->peerip), &peer, CON_ACTIVE, IDLE);
		if (sub->oper_type != OPER_GET_REQ && OPER_FROM_POSS != sub->oper_type)
		{
			LOG(pfs_data_log, LOG_ERROR, "%s:%d ERROR oper_type %d %s!\n", ID, LN, sub->oper_type, base->filename);
			base->overstatus = OVER_E_TYPE;
			pfs_set_task(task, TASK_Q_FIN);
			continue;
		}
		LOG(pfs_data_log, LOG_DEBUG, "%s is prepare OPER_GET_REQ from %s\n", base->filename, sub->peerip);
		if(check_disk_space(base) != DISK_OK)
		{
			LOG(pfs_data_log, LOG_DEBUG, "%s:%d filename[%s] DISK NOT ENOUGH SPACE!\n", ID, LN, base->filename);
			if (DISK_SPACE_TOO_SMALL == check_disk_space(base))
			{
				if (sub->need_sync == TASK_SYNC_ISDIR)
					pfs_set_task(task, TASK_Q_SYNC_DIR_TMP);
				else
					pfs_set_task(task, TASK_Q_WAIT_TMP);
			}
			else
			{
				base->overstatus = OVER_DISK_ERR;
				pfs_set_task(task, TASK_Q_FIN);
			}
			continue;
		}

		if (peer == NULL)
		{
			int count = get_ip_connect_count(str2ip(sub->peerip));
			if (count > g_config.max_data_connects)
			{
				LOG(pfs_data_log, LOG_DEBUG, "ip %s too many connect %d max %d\n", sub->peerip, count, g_config.max_data_connects);
				if (sub->need_sync == TASK_SYNC_ISDIR)
					pfs_set_task(task, TASK_Q_SYNC_DIR_TMP);
				else
					pfs_set_task(task, TASK_Q_WAIT_TMP);
				continue;
			}
			active_connect(sub->peerip);
			char *peerhost = sub->peerip;

			if (find_ip_stat(str2ip(peerhost), &peer, CON_ACTIVE, IDLE) != 0)
			{
				LOG(pfs_data_log, LOG_DEBUG, "%s be hung up because find_ip_stat error!\n", base->filename);
				if (sub->need_sync == TASK_SYNC_ISDIR)
					pfs_set_task(task, TASK_Q_SYNC_DIR_TMP);
				else
					pfs_set_task(task, TASK_Q_WAIT_TMP);
				continue;
			}
		}
		if (g_config.pfs_test)
		{
			LOG(pfs_data_log, LOG_NORMAL, "pfs run in test %s\n", base->filename);
			base->overstatus = OVER_OK;
			pfs_set_task(task, TASK_Q_FIN);
			continue;
		}
		t_pfs_sig_head h;
		t_pfs_sig_body b;
		sub->lastlen = 0;
		h.bodylen = sizeof(t_task_base);
		memcpy(b.body, base, sizeof(t_task_base));
		h.cmdid = CMD_GET_FILE_REQ;
		h.status = FILE_SYNC_DST_2_SRC;
		active_send(peer, &h, &b);
		peer->sock_stat = PREPARE_RECVFILE;
		LOG(pfs_data_log, LOG_DEBUG, "fd[%d:%s] %s send get a file sock_stat [%s]!\n", peer->fd, sub->peerip, base->filename, sock_stat_cmd[peer->sock_stat]);
		pfs_set_task(task, TASK_Q_RECV);
		peer->recvtask = task;
	}
}


