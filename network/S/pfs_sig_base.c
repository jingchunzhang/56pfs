/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
/*
 *CS FCS ip信息采用uint32_t 存储，便于存储和查找
 */
#include "pfs_file_filter.h"
volatile extern int maintain ;		//1-维护配置 0 -可以使用
extern t_pfs_nameserver pfs_nameserver[MAX_NAMESERVER];
extern t_pfs_domain *self_domain;

static void do_sync_dir_req_sub(t_task_base *sbase)
{
	t_pfs_sync_task *task = (t_pfs_sync_task *) sbase;

	char *datadir = myconfig_get_value("pfs_datadir");
	if (datadir == NULL)
		datadir = "/diska/m2v/photo2video/upImg/";
	char realpath[256] = {0x0};
	snprintf(realpath, sizeof(realpath), "%s/%d/%d", datadir, task->d1, task->d2);

	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(realpath)) == NULL) 
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "opendir %s err  %m\n", realpath);
		sbase->overstatus = OVER_E_OPEN_SRCFILE;
	}
	else
	{
		while((dirp = readdir(dp)) != NULL) 
		{
			if (dirp->d_name[0] == '.')
				continue;

			if (check_file_filter(dirp->d_name))
			{
				LOG(pfs_sig_log, LOG_TRACE, "syncdir req filename %s check not ok!\n", dirp->d_name);
				continue;
			}
			char file[256] = {0x0};
			snprintf(file, sizeof(file), "%s/%s", realpath, dirp->d_name);

			struct stat filestat;
			if(stat(file, &filestat) < 0) 
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "stat error,filename:%s\n", file);
				continue;
			}
			if (!S_ISREG(filestat.st_mode))
				continue;
			if (task->starttime && filestat.st_ctime < task->starttime)
				continue;
			if (task->endtime && filestat.st_ctime > task->endtime)
				continue;
			if (access(file, R_OK))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "file access error,filename:%s:%m\n", file);
				continue;
			}

			t_task_base base;
			memset(&base, 0, sizeof(base));
			snprintf(base.filename, sizeof(base.filename), "%s/%s", realpath, dirp->d_name);

			base.fsize = filestat.st_size;
			base.mtime = filestat.st_mtime;

			unsigned char md5[17] = {0x0};
			if (getfilemd5(file, md5))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "md5 error,filename:%s %m\n", file);
				continue;
			}
			memcpy(base.filemd5, md5, sizeof(base.filemd5));
			base.type = TASK_ADDFILE;

			t_pfs_tasklist *stask = NULL;
			if (pfs_get_task(&stask, TASK_Q_HOME))
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "pfs_get_task from TASK_Q_HOME ERR %m\n");
				continue;
			}

			memset(&(stask->task), 0, sizeof(stask->task));
			ip2str(stask->task.sub.peerip, sbase->dstip);
			memcpy(&(stask->task.base), &base, sizeof(t_task_base));
			pfs_set_task(stask, TASK_Q_WAIT_SYNC_DIR);  
		}
		closedir(dp);
		sbase->overstatus = OVER_OK;
	}
}

static void active_connect(char *ip)
{
	t_pfs_sig_head h;
	int port = g_config.sig_port;
	h.status = C_A_2_T;

	int fd = createsocket(ip, port);
	if (fd < 0)
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "connect %s:%d err %m\n", ip, port);
		return;
	}
	if (svc_initconn(fd))
	{
		LOG(pfs_sig_log_err, LOG_ERROR, "svc_initconn err %m\n");
		close(fd);
		return;
	}
	add_fd_2_efd(fd);
	LOG(pfs_sig_log, LOG_NORMAL, "connect %s:%d\n", ip, port);
}

/*find 活动链接信息 */
int find_ip_stat(uint32_t ip, pfs_cs_peer **dpeer)
{
	pfs_cs_peer *peer = NULL;
	list_head_t *l;
	list_head_t * hashlist = &(online_list[ALLMASK&ip]);
	peer = NULL;
	list_for_each_entry_safe_l(peer, l, hashlist, hlist)
	{
		if (peer->ip == ip)
		{
			*dpeer = peer;
			return 0;
		}
	}
	return -1;
}

static void scan_cfg_iplist_and_connect()
{
	pfs_cs_peer *peer = NULL;
	int i;
	for (i =0; i < MAX_NAMESERVER; i++)
	{
		if (pfs_nameserver[i].uip == 0)
			break;
		if (find_ip_stat(pfs_nameserver[i].uip, &peer))
			active_connect(pfs_nameserver[i].ip);
	}

	for (i = 0; i < self_domain->ipcount; i++)
	{
		if (self_domain->self[i])
			continue;
		if (self_domain->offline[i])
			continue;
		if (find_ip_stat(self_domain->ips[i], &peer))
			active_connect(self_domain->sips[i]);
	}
}

static void do_sub_sync(t_pfs_sync_list *pfs_sync, pfs_cs_peer *peer)
{
	char sip[16] = {0x0};
	ip2str(sip, peer->ip);
	int d1 = pfs_sync->sync_task.d1;
	int d2 = pfs_sync->sync_task.d2;
	LOG(pfs_sig_log, LOG_NORMAL, "%s:%s:%d  sync [%d:%d] to [%s]!\n", ID, FUNC, LN, d1, d2, sip);
	char obuf[2048] = {0x0};
	int n = 0;
	if (pfs_sync->sync_task.type == TASK_ADDFILE)
		n = create_sig_msg(SYNC_DIR_REQ, TASK_SYNC, (t_pfs_sig_body *)&(pfs_sync->sync_task), obuf, sizeof(pfs_sync->sync_task));
	else
		n = create_sig_msg(SYNC_DEL_REQ, TASK_SYNC, (t_pfs_sig_body *)&(pfs_sync->sync_task), obuf, sizeof(pfs_sync->sync_task));
	set_client_data(peer->fd, obuf, n);
	modify_fd_event(peer->fd, EPOLLOUT);
	peer->pfs_sync_list = pfs_sync;
	sync_para.flag = 1;
	sync_para.last = time(NULL);
}

static void do_active_sync()
{
	int run_sync_dir_count = get_task_count(TASK_Q_SYNC_DIR) + get_task_count(TASK_Q_SYNC_DIR_TMP);
	if (run_sync_dir_count >= 1024)
	{
		LOG(pfs_sig_log, LOG_NORMAL, "so many sync_dir_task %d\n", run_sync_dir_count);
		return;
	}
	char curtime[16] = {0x0};
	get_strtime(curtime);
	char hour[12] = {0x0};
	snprintf(hour, sizeof(hour), "%.2s:%.2s:%.2s", curtime+8, curtime+10, curtime+12);
	if (strcmp(hour, g_config.sync_stime) < 0 || strcmp(hour, g_config.sync_etime) > 0)
	{
		LOG(pfs_sig_log, LOG_DEBUG, "cur time %s not between %s %s, can not sync dir\n", hour, g_config.sync_stime, g_config.sync_etime);
		return;
	}
	t_pfs_sync_list *pfs_sync = NULL;
	list_head_t *l;
	int get = 0;
	list_for_each_entry_safe_l(pfs_sync, l, &sync_list, list)
	{
		get = 1;
		break;
	}
	if (get == 0)
	{
		LOG(pfs_sig_log, LOG_NORMAL, "no sync_task in list!\n");
		sync_para.flag = 2;
		return;
	}
	int d1 = pfs_sync->sync_task.d1;
	int d2 = pfs_sync->sync_task.d2;
	LOG(pfs_sig_log, LOG_NORMAL, "get sync task %d %d\n", d1, d2);
	pfs_cs_peer *peer = NULL;
	int i;
	int self_leader = 1;
	for (i = 0; i < self_domain->ipcount; i++)
	{
		if (self_domain->self[i])
			continue;
		if (self_domain->offline[i])
			continue;
		if (self_domain->ips[i] < self_ip)
			self_leader = 0;
		if (find_ip_stat(self_domain->ips[i], &peer))
		{
			LOG(pfs_sig_log, LOG_ERROR, "ip not online %s\n", self_domain->sips[i]);
			continue;
		}
		do_sub_sync(pfs_sync, peer);
		list_del_init(&(pfs_sync->list));
	}
}

static int init_sync_list()
{
	int d1, d2;
	time_t cur = time(NULL);
	t_pfs_sync_list *pfs_sync;  
	for (d1 = 0; d1 < DIR1; d1++)
	{
		for (d2 = 0; d2 < DIR1; d2++)
		{
			time_t last = get_time_stamp_by_int(d1, d2) - g_config.task_timeout;
			if (last <= 0)
				last = 0;
				//last = get_storage_dir_lasttime(d1, d2);
			pfs_sync = malloc(sizeof(t_pfs_sync_list));
			if (pfs_sync == NULL)
			{
				LOG(pfs_sig_log_err, LOG_ERROR, "ERROR malloc %m\n");
				return -1;
			}
			memset(pfs_sync, 0, sizeof(t_pfs_sync_list));
			INIT_LIST_HEAD(&(pfs_sync->list));
			pfs_sync->sync_task.starttime = last;
			pfs_sync->sync_task.endtime = cur;
			pfs_sync->sync_task.d1 = d1;
			pfs_sync->sync_task.d2 = d2;
			pfs_sync->sync_task.type = TASK_ADDFILE;
			LOG(pfs_sig_log, LOG_NORMAL, "gen sync task %d %d %ld %s\n", d1, d2, last, ctime(&last));
			list_add(&(pfs_sync->list), &sync_list);
		}
	}
	return 0;
}

uint32_t getloadinfo()
{
	char *file = "/proc/loadavg";
	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		LOG(pfs_sig_log, LOG_ERROR, "open %s err %m\n", file);
		return 0;
	}

	char buf[1024] = {0x0};
	while(fgets(buf, sizeof(buf), fp))
	{
		char *t = strchr(buf, ' ');
		if (t)
			*t = 0x0;
		break;
	}
	fclose(fp);
	double w = atof(buf) * (double )(100);
	return (uint32_t)w;
}
