/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_task.h"
#include "log.h"
#include "common.h"
#include "pfs_timer.h"
#include "pfs_init.h"
#include "c_api.h"
#include "atomic.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
extern int glogfd;
extern t_g_config g_config;

#define TASK_Q_HASHSIZE 16
#define TASK_Q_MOD 0x0F

atomic_t taskcount[TASK_Q_UNKNOWN];

static pthread_mutex_t mutex[TASK_Q_UNKNOWN];
static list_head_t pfstask[TASK_Q_UNKNOWN];  /*sub task status ,just for cs or for tracker by cfg*/
uint64_t need_send_bytes;  /*等待发送的字节数*/
uint64_t been_send_bytes;  /*已经发送的字节数*/

const char *task_status[TASK_Q_UNKNOWN] = {"TASK_Q_DELAY", "TASK_Q_WAIT", "TASK_Q_WAIT_TMP", "TASK_Q_WAIT_SYNC", "TASK_Q_WAIT_SYNC_IP", "TASK_Q_WAIT_SYNC_DIR", "TASK_Q_WAIT_SYNC_DIR_TMP", "TASK_Q_SYNC_DIR", "TASK_Q_SYNC_DIR_TMP", "TASK_Q_RUN", "TASK_Q_FIN", "TASK_Q_CLEAN", "TASK_Q_HOME", "TASK_Q_SEND", "TASK_Q_RECV", "TASK_Q_SYNC_VOSS", "TASK_Q_SYNC_DIR_REQ", "TASK_Q_SYNC_DIR_RSP"};
const char *over_status[OVER_LAST] = {"OVER_UNKNOWN", "OVER_OK", "OVER_E_MD5", "OVER_PEERERR", "TASK_Q_EXIST", "OVER_PEERCLOSE", "OVER_UNLINK", "OVER_TIMEOUT", "OVER_MALLOC", "OVER_SRC_DOMAIN_ERR", "OVER_SRC_IP_OFFLINE", "OVER_E_OPEN_SRCFILE", "OVER_E_OPEN_DSTFILE", "OVER_E_IP", "OVER_E_TYPE", "OVER_SEND_LEN", "OVER_TOO_MANY_TRY", "OVER_DISK_ERR"};
/*
 * every status have a list
 * 0:wait
 * 1:runing
 * 2:fin
 * 3:all
 */

int pfs_get_task(t_pfs_tasklist **task, int status)
{
	int ret = GET_TASK_Q_ERR;
	if (status < 0 || status >= TASK_Q_UNKNOWN)
	{
		LOG(glogfd, LOG_ERROR, "ERR %s:%d status range error %d\n", FUNC, LN, status);
		return ret;
	}
	struct timespec to;
	to.tv_sec = g_config.lock_timeout + time(NULL);
	to.tv_nsec = 0;
	ret = pthread_mutex_timedlock(&mutex[status], &to);
	if (ret != 0)
	{
		if (ret != EDEADLK)
		{
			LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_timedlock error %d\n", FUNC, LN, ret);
			report_err_2_nm(ID, FUNC, LN, ret);
			return ret;
		}
	}

	ret = GET_TASK_Q_NOTHING;
	t_pfs_tasklist *task0 = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(task0, l, &pfstask[status], llist)
	{
		atomic_dec(&(taskcount[status]));
		ret = GET_TASK_Q_OK;
		*task = task0;
		(*task)->status = TASK_Q_UNKNOWN;
		list_del_init(&(task0->llist));
		break;
	}
	if (ret == GET_TASK_Q_NOTHING && status == TASK_Q_HOME)
	{
		LOG(glogfd, LOG_DEBUG, "get from home , need malloc!\n");
		*task = (t_pfs_tasklist *) malloc(sizeof(t_pfs_tasklist));
		if (*task == NULL)
			LOG(glogfd, LOG_ERROR, "ERR %s:%d malloc %m\n", FUNC, LN);
		else
		{
			ret = GET_TASK_Q_OK;
			INIT_LIST_HEAD(&((*task)->llist));
			INIT_LIST_HEAD(&((*task)->userlist));
			(*task)->last = time(NULL);
			(*task)->status = TASK_Q_UNKNOWN;
		}
	}

	if (pthread_mutex_unlock(&mutex[status]))
		LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_unlock error %m\n", FUNC, LN);
	return ret;
}

int pfs_set_task(t_pfs_tasklist *task, int status)
{
	if (status < 0 || status >= TASK_Q_UNKNOWN)
	{
		LOG(glogfd, LOG_ERROR, "ERR %s:%d status range error %d\n", FUNC, LN, status);
		return -1;
	}
	int ret = -1;
	struct timespec to;
	to.tv_sec = g_config.lock_timeout + time(NULL);
	to.tv_nsec = 0;
	ret = pthread_mutex_timedlock(&mutex[status], &to);
	if (ret != 0)
	{
		if (ret != EDEADLK)
		{
			LOG(glogfd, LOG_ERROR, "ERR %s:%d [%s] pthread_mutex_timedlock error %d\n", FUNC, LN, task_status[status], ret);
			report_err_2_nm(ID, FUNC, LN, ret);
			return -1;
		}
	}

	list_del_init(&(task->llist));
	list_add_tail(&(task->llist), &pfstask[status]);
	task->status = status;
	if (task->last > 0)
		task->last = time(NULL);
	atomic_inc(&(taskcount[status]));

	if (pthread_mutex_unlock(&mutex[status]))
		LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_unlock error %m\n", FUNC, LN);
	return ret;
}

int init_task_info()
{
	int i = 0;
	for (i = 0; i < TASK_Q_UNKNOWN; i++)
	{
		INIT_LIST_HEAD(&pfstask[i]);
		if (pthread_mutex_init(&mutex[i], NULL))
		{
			LOG(glogfd, LOG_ERROR, "pthread_mutex_init [%s] err %m\n", task_status[i]);
			report_err_2_nm(ID, FUNC, LN, 0);
			return -1;
		}
	}

	t_pfs_tasklist *taskall = (t_pfs_tasklist *) malloc (sizeof(t_pfs_tasklist) * 2048);
	if (taskall == NULL)
	{
		LOG(glogfd, LOG_ERROR, "malloc t_pfs_tasklist error %m\n");
		return -1;
	}
	memset(taskall, 0, sizeof(t_pfs_tasklist) * 2048);

	for (i = 0; i < 2048; i++)
	{
		INIT_LIST_HEAD(&(taskall->llist));
		INIT_LIST_HEAD(&(taskall->userlist));
		list_add(&(taskall->llist), &pfstask[TASK_Q_HOME]);
		taskall->last = -1;
		taskall->status = TASK_Q_HOME;
		taskall++;
	}

	memset(taskcount, 0, sizeof(taskcount));
	t_pfs_timer pfs_timer;
	memset(&pfs_timer, 0, sizeof(pfs_timer));
	pfs_timer.loop = 1;
	pfs_timer.span_time = 60;
	pfs_timer.cb = report_2_nm;
	if (add_to_delay_task(&pfs_timer))
		LOG(glogfd, LOG_ERROR, "add_to_delay_task err %m\n");
	LOG(glogfd, LOG_DEBUG, "init_task_info ok!\n");
	return 0;
}

int scan_some_status_task(int status, timeout_task cb)
{
	if (status < 0 || status >= TASK_Q_UNKNOWN)
	{
		LOG(glogfd, LOG_ERROR, "ERR %s:%d status range error %d\n", FUNC, LN, status);
		return -1;
	}
	int ret = -1;
	time_t cur = time(NULL);
	struct timespec to;
	to.tv_sec = g_config.lock_timeout + cur;
	to.tv_nsec = 0;
	ret = pthread_mutex_timedlock(&mutex[status], &to);
	if (ret != 0)
	{
		if (ret != EDEADLK)
		{
			LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_timedlock error %d\n", FUNC, LN, ret);
			report_err_2_nm(ID, FUNC, LN, ret);
			return -1;
		}
	}

	t_pfs_tasklist *task0 = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(task0, l, &pfstask[status], llist)
	{
		cb(task0);
	}

	if (pthread_mutex_unlock(&mutex[status]))
		LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_unlock error %m\n", FUNC, LN);
	return ret;
}

inline int get_task_count(int status)
{
	if (status < 0 || status >= TASK_Q_UNKNOWN)
	{
		LOG(glogfd, LOG_ERROR, "ERR %s:%d status range error %d\n", FUNC, LN, status);
		return -1;
	}
	return atomic_read(&(taskcount[status]));
}

void report_2_nm()
{
	char buf[256] = {0x0};
	snprintf(buf, sizeof(buf), "pfs_test=%u|task_timeout=%ld|task_run=%d|task_wait=%d|task_fin=%d|task_clean=%d|task_wait_sync=%d|task_wait_sync_ip=%d|task_wait_tmp=%d|", g_config.pfs_test, g_config.task_timeout, get_task_count(TASK_Q_RUN), get_task_count(TASK_Q_WAIT), get_task_count(TASK_Q_FIN), get_task_count(TASK_Q_CLEAN), get_task_count(TASK_Q_WAIT_SYNC), get_task_count(TASK_Q_WAIT_SYNC_IP), get_task_count(TASK_Q_WAIT_TMP)); 

	int rulebase = PFS_TASK_DEPTH_BASE;
	int totaltask = 0;
	int i = 0;
	for (i = TASK_Q_DELAY ; i < TASK_Q_HOME; i++)
	{
		totaltask += get_task_count(i);
		rulebase++;
	}
	LOG(glogfd, LOG_NORMAL, "report 2 nm %s  %d\n", buf, totaltask);
}

void dump_task_info (int logfd, char *from, int line, t_task_base *task)
{
	char ip[16] = {0x0};
	if (task->dstip)
	{
		ip2str(ip, task->dstip);
		LOG(logfd, LOG_DEBUG, "from %s:%d filename [%s] filesize[%ld] type [%c] dstip[%s]\n", from, line, task->filename, task->fsize, task->type, ip);
	}
	else
		LOG(logfd, LOG_DEBUG, "from %s:%d filename [%s] filesize[%ld] type [%c] dstip is null\n", from, line, task->filename, task->fsize, task->type);
}

static void check_task_timeout(t_pfs_tasklist *task)
{
	time_t cur = time(NULL);
	t_task_base *base = &(task->task.base);
	if (cur - base->starttime > g_config.task_timeout)
		base->overstatus = OVER_TIMEOUT;

	char ip[16] = {0x0};
	ip2str(ip, base->dstip);
	if (base->overstatus == OVER_TIMEOUT)
	{
		LOG(glogfd, LOG_DEBUG, "move task [%s:%s] to home\n", base->filename, over_status[base->overstatus]);
		list_del_init(&(task->llist));
		list_del_init(&(task->userlist));
		atomic_dec(&(taskcount[task->status]));
		memset(&(task->task), 0, sizeof(task->task));
		pfs_set_task(task, TASK_Q_HOME);
	}
}

void do_timeout_task()
{
	int i = 0;
	for (i = TASK_Q_DELAY ; i < TASK_Q_CLEAN; i++)
	{
		scan_some_status_task(i, check_task_timeout);
	}
}


