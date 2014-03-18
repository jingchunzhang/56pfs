/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include "pfs_maintain.h"
#include "pfs_init.h"
#include "myconfig.h"
#include "mybuff.h"
#include "pfs_task.h"
#include "pfs_time_stamp.h"
#include "common.h"
#include "version.h"
#include "log.h"
#include "util.h"
extern int pfs_agent_log ;
extern char *iprole[]; 
extern time_t pfs_start_time;  /*pfs Æô¶¯Ê±¼ä*/

static char *cmd_type[] = {"M_ONLINE", "M_OFFLINE", "M_GETINFO", "M_SYNCDIR", "M_SYNCFILE", "M_CONFUPDA", "M_SETDIRTIME", "M_GETDIRTIME", "M_ADD_MOD_GROUP", "M_DEL_GROUP"};

static char *validcmd[] = {"cs_preday", "fcs_max_connects", "max_data_connects", "max_task_run_once", "pfs_test", "real_rm_time", "task_timeout", "fcs_max_task", "cs_sync_dir", "data_calcu_md5", "sync_dir_count", "timeout"};
#define validsize sizeof(validcmd)/sizeof(char*)

static int isvalid(char *key)
{
	int i = 0;
	for (i = 0; i < validsize; i++)
	{
		if (strcasecmp(key, validcmd[i]) == 0)
			return 0;
	}
	return 1;
}

static int do_confupda(StringPairList *pairlist)
{
	char *p;
	int i = 0;
	int n = 0;
    for (i = 0; i < pairlist->iLast; i++ )
	{
		char *key = pairlist->pStrPairList[i].sFirst;
		char *val = pairlist->pStrPairList[i].sSecond;
		if(strcasecmp(key, "pfs_cmd") == 0)
			continue;
		if(isvalid(key))
		{
			LOG(pfs_agent_log, LOG_ERROR, "ERROR CMD %s VALUE %s\n", key, val);
			continue;
		}
		p = pairlist->pStrPairList[i].sSecond;
		n = strlen(pairlist->pStrPairList[i].sSecond);
		while(--n)
		{
			if(*p == '\r' || *p == '\n')
			{
				*p = 0;
				break;
			}
			p++;
		}
		char *bkval = myconfig_get_value(key);
		myconfig_delete_value("", key);
		LOG(pfs_agent_log, LOG_NORMAL, "update key = %s val = %s, old val is %s\n", key, val, bkval?bkval:"nothing");
		if(myconfig_update_value(key, val) < 0)
			return -1;
	}
	myconfig_dump_to_file();
	reload_config();
	return 0;
}

static inline int get_cmd_position(char *cmd)
{
	int i = 0;
	for(i = 0; i < INVALID; i++)
	{
		if(strcmp(cmd, cmd_type[i]) == 0)
			return i;
	}
	return -1;
}

int get_info(char *buf,  int len)
{
	char stime[16] = {0x0};
	get_strtime_by_t(stime, pfs_start_time);
	return snprintf(buf, len, "starttime=%s|compile_time=%s %s|pfs_test=%u|task_timeout=%ld|task_run=%d|task_wait=%d|task_fin=%d|task_clean=%d|task_wait_sync=%d|task_wait_sync_ip=%d|task_wait_tmp=%d|", stime, compiling_date, compiling_time, g_config.pfs_test, g_config.task_timeout, get_task_count(TASK_Q_RUN), get_task_count(TASK_Q_WAIT), get_task_count(TASK_Q_FIN), get_task_count(TASK_Q_CLEAN), get_task_count(TASK_Q_WAIT_SYNC), get_task_count(TASK_Q_WAIT_SYNC_IP), get_task_count(TASK_Q_WAIT_TMP)); 
}

int set_dirtime(StringPairList *pairlist, char *buf, int len)
{
	char *p;
	int i = 0;
	int n = 0;
	int ol = 0;
	for (i = 0; i < pairlist->iLast; i++ )
	{
		char *key = pairlist->pStrPairList[i].sFirst;
		char *val = pairlist->pStrPairList[i].sSecond;
		if(strcasecmp(key, "pfs_cmd") == 0)
			continue;
		p = pairlist->pStrPairList[i].sSecond;
		n = strlen(pairlist->pStrPairList[i].sSecond);
		while(--n)
		{
			if(*p == '\r' || *p == '\n')
			{
				*p = 0;
				break;
			}
			p++;
		}
		int d1 = atoi(key);
		p = strchr(key, ',');
		if (p == NULL)
		{
			LOG(pfs_agent_log, LOG_ERROR, "err data format %s=%s\n", key, val);
			continue;
		}
		p++;
		int d2 = atoi(p);
		time_t tval = get_time_t (val);
		time_t oval = 0;
		oval = get_time_stamp_by_int(d1, d2);
		set_time_stamp_by_int(d1, d2, tval);
		char stime[16] = {0x0};
		get_strtime_by_t(stime, oval);
		LOG(pfs_agent_log, LOG_NORMAL, "set fcs %s = %s(%ld), old %s(%ld)\n", key, val, tval, stime, oval);
		if (ol < len)
			ol += snprintf(buf+ol, len-ol, "set fcs %s=%s(%ld),old %s(%ld) ", key, val, tval, stime, oval);
	}
	ol += snprintf(buf+ol, len-ol, "\n");
	return ol;
}

int get_dirtime(char *buf, int len)
{
	return get_dirtime(buf, len);
}

int do_poss_sync_file(char *file, char *sip)
{
	t_pfs_tasklist *task = NULL;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_agent_log, LOG_ERROR, "pfs_get_task from TASK_Q_HOME ERR!\n");
		return -1;
	}
	t_task_base *base = (t_task_base*)(&task->task.base);
	t_task_sub *sub = (t_task_sub*)(&task->task.sub);
	memset(base, 0, sizeof(t_task_base));
	memset(sub, 0, sizeof(t_task_sub));
	snprintf(base->filename, sizeof(base->filename), "%s", file);
	base->starttime = time(NULL);
	base->type = TASK_ADDFILE;
	base->dstip = self_ip;
	sub->oper_type = OPER_FROM_POSS;
	sub->need_sync = TASK_SYNC_VOSS_FILE;
	snprintf(sub->peerip, sizeof(sub->peerip), "%s", sip);
	pfs_set_task(task, TASK_Q_WAIT);
	return 0;
}

int do_poss_sync_dir(char *domain, char *file, time_t starttime)
{
	t_pfs_tasklist *task = NULL;
	if (pfs_get_task(&task, TASK_Q_HOME))
	{
		LOG(pfs_agent_log, LOG_ERROR, "pfs_get_task from TASK_Q_HOME ERR!\n");
		return -1;
	}
	t_task_base *base = (t_task_base*)(&task->task.base);
	t_task_sub *sub = (t_task_sub*)(&task->task.sub);
	memset(base, 0, sizeof(t_task_base));
	memset(sub, 0, sizeof(t_task_sub));
	snprintf(base->filename, sizeof(base->filename), "%s", file);
	base->starttime = starttime;
	base->type = TASK_SYNCDIR;
	sub->need_sync = TASK_SYNC_VOSS_FILE;
	pfs_set_task(task, TASK_Q_SYNC_POSS);
	return 0;
}

int poss_sync_file(StringPairList *pairlist, char *buf, int len)
{
	char *p;
	int i = 0;
	int n = 0;
	int ol = 0;
    for (i = 0; i < pairlist->iLast; i++ )
	{
		char *key = pairlist->pStrPairList[i].sFirst;
		char *val = pairlist->pStrPairList[i].sSecond;
		if(strcasecmp(key, "pfs_cmd") == 0)
			continue;
		p = pairlist->pStrPairList[i].sSecond;
		n = strlen(pairlist->pStrPairList[i].sSecond);
		while(--n)
		{
			if(*p == '\r' || *p == '\n')
			{
				*p = 0;
				break;
			}
			p++;
		}
		LOG(pfs_agent_log, LOG_NORMAL, "%s=%s\n", key, val);
		if (do_poss_sync_file(key, val))
		{
			if (ol < len)
				ol += snprintf(buf + ol, len - ol, "sync %s %s err %m", key, val);
		}
	}
	return ol;
}

int poss_sync_dir(StringPairList *pairlist, char *buf, int len)
{
	char *p;
	int i = 0;
	int n = 0;
	int ol = 0;
    for (i = 0; i < pairlist->iLast; i++ )
	{
		char *key = pairlist->pStrPairList[i].sFirst;
		char *val = pairlist->pStrPairList[i].sSecond;
		if(strcasecmp(key, "pfs_cmd") == 0)
			continue;
		p = pairlist->pStrPairList[i].sSecond;
		n = strlen(pairlist->pStrPairList[i].sSecond);
		while(--n)
		{
			if(*p == '\r' || *p == '\n')
			{
				*p = 0;
				break;
			}
			p++;
		}
		p = strrchr(val, '|');
		if (p == NULL)
		{
			if (ol < len)
				ol += snprintf(buf + ol, len - ol, "sync %s %s errformat domain=d1,d2|yyyymmddHHMMSS", key, val);
			continue;
		}
		*p = 0x0;
		p++;
		time_t starttime = get_time_t(p);
		LOG(pfs_agent_log, LOG_NORMAL, "%s=%s stime=%s:%ld\n", key, val, p, starttime);
		if (do_poss_sync_dir(key, val, starttime))
		{
			if (ol < len)
				ol += snprintf(buf + ol, len - ol, "sync %s %s err %m", key, val);
		}
	}
	return ol;
}

int poss_off_on_line(StringPairList *pairlist, char *buf, int len, int type)
{
	char *p;
	int i = 0;
	int n = 0;
	int ol = 0;
	char sip[16] = {0x0};
	uint32_t ip = 0;
    for (i = 0; i < pairlist->iLast; i++ )
	{
		char *key = pairlist->pStrPairList[i].sFirst;
		char *v = pairlist->pStrPairList[i].sSecond;
		if(strcasecmp(key, "pfs_cmd") == 0)
			continue;
		p = pairlist->pStrPairList[i].sSecond;
		n = strlen(pairlist->pStrPairList[i].sSecond);
		while(--n)
		{
			if(*p == '\r' || *p == '\n')
			{
				*p = 0;
				break;
			}
			p++;
		}
		while (1)
		{
			char *t = strchr(v, ',');
			if (t == NULL)
				break;
			*t = 0x0;
			ip = get_uint32_ip(v, sip);
			v = t + 1;
		}
		ip = get_uint32_ip(v, sip);
	}
	return ol;
}

void do_request(int fd, int datalen, char *data)
{
	LOG(pfs_agent_log, LOG_NORMAL, "Start process request [%.*s]\n", datalen, data);
	if(!data)
	{
		LOG(pfs_agent_log, LOG_ERROR, "data is null!\n");
		return;
	}

	StringPairList *pairlist = CreateStringPairList(20);
	if (!pairlist)
	{
		LOG(pfs_agent_log, LOG_ERROR, "malloc err %m\n");
		return;
	}

	if(DecodePara(data, datalen, pairlist) != 0)
	{
		DestroyStringPairList(pairlist);
		LOG(pfs_agent_log, LOG_NORMAL, "DecodePara error [%.*s]\n", datalen, data);
		return;
	}

	const char * cmd_key = "pfs_cmd";
	char cmd_value[32] = {0x0};
	if(!GetParaValue(pairlist, cmd_key, cmd_value, sizeof(cmd_value)))
	{
		DestroyStringPairList(pairlist);
		LOG(pfs_agent_log, LOG_NORMAL, "GetParaValue error [%.*s]\n", datalen, data);
		return;
	}
	char *p = cmd_value + strlen(cmd_value);
	while (p--)
	{
		if (isalpha(*p) || isdigit(*p))
			break;
		*p = 0x0;
	}

	int type = get_cmd_position(cmd_value);

	if (type == -1)
	{
		DestroyStringPairList(pairlist);
		LOG(pfs_agent_log, LOG_NORMAL, "INVALID_CMD [%s][%s]\n", cmd_value, data);
		return;
	}

	char sendbuf[40960] = {0x0};
	int sendlen = 0;
	switch(type)
	{
		case M_CONFUPDA:
			if(do_confupda(pairlist) == 0)
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "%s::Result=OK&Code=0&Msg=Success", data);
			else
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "%s::Msg=Failed", data);
			break;
		case M_GETINFO:
			sendlen = get_info(sendbuf, sizeof(sendbuf));
			break;
		case M_SETDIRTIME:
			sendlen = set_dirtime(pairlist, sendbuf, sizeof(sendbuf));
			break;
		case M_GETDIRTIME:
			sendlen = get_dirtime(sendbuf, sizeof(sendbuf));
			break;
		case M_SYNCFILE:
			sendlen = poss_sync_file(pairlist, sendbuf, sizeof(sendbuf));
			if (sendlen == 0)
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "sync file OK!");
			break;
		case M_SYNCDIR:
			sendlen = poss_sync_dir(pairlist, sendbuf, sizeof(sendbuf));
			if (sendlen == 0)
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "sync dir OK!");
			break;

		case M_OFFLINE:
			sendlen = poss_off_on_line(pairlist, sendbuf, sizeof(sendbuf), M_OFFLINE);
			if (sendlen == 0)
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "offline %s OK!", data);
			break;

		case M_ONLINE:
			sendlen = poss_off_on_line(pairlist, sendbuf, sizeof(sendbuf), M_ONLINE);
			if (sendlen == 0)
				sendlen = snprintf(sendbuf, sizeof(sendbuf), "online %s OK!", data);
			break;

		default:
			sendlen = snprintf(sendbuf, sizeof(sendbuf), "%s:: not implement", data);
			break;
	}
	DestroyStringPairList(pairlist);
	if (sendlen <= 0)
		return;
	LOG(pfs_agent_log, LOG_NORMAL, "return [%s]\n", sendbuf);
	struct conn *curcon = &acon[fd];
	t_head_info head;
	memset(&head, 0, sizeof(head));
	create_poss_head((char *)&head, RSP_PFS_CMD, sendlen);
	mybuff_setdata(&(curcon->send_buff), (char *)&head, sizeof(head));
	mybuff_setdata(&(curcon->send_buff), sendbuf, sendlen);
	return;
}

