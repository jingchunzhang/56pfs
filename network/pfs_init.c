/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_init.h"
#include "pfs_so.h"
#include "pfs_file_filter.h"
#include "pfs_localfile.h"
#include "log.h"
#include "common.h"
#include "thread.h"
#include "pfs_maintain.h"
#include "acl.h"
#include "pfs_timer.h"
#include <stddef.h>
#include <sys/vfs.h> 

int DIR1 ;
int DIR2 ;

static pthread_mutex_t cfgmutex = PTHREAD_MUTEX_INITIALIZER;
int start_rand = 0;
extern int init_buff_size;
uint32_t maxfreevalue;  /*Gbytes*/

const char *path_dirs[] = {"indir", "outdir", "workdir", "bkdir", "tmpdir"};

const char *name_policy[] = {"diskfree", "load", "taskcount"};

t_pfs_nameserver pfs_nameserver[MAX_NAMESERVER];
t_g_config g_config;

list_head_t grouplist;
#define IPLIST 32
#define IPMODE 0x1F
list_head_t allip[IPLIST];
static list_head_t alldomain;

extern t_pfs_domain *self_domain;
static t_disk_info diskinfo[MAXDISKS];
static t_disk_info *max_disk = NULL;

static uint32_t localip[64];
char possip[64];
uint16_t possport = 0;

static int init_local_ip()
{
	INIT_LIST_HEAD(&grouplist);
	memset(localip, 0, sizeof(localip));
	struct ifconf ifc;
	struct ifreq *ifr = NULL;
	int i;
	int nifr = 0;

	i = socket(AF_INET, SOCK_STREAM, 0);
	if(i < 0) 
		return -1;
	ifc.ifc_len = 0;
	ifc.ifc_req = NULL;
	if(ioctl(i, SIOCGIFCONF, &ifc) == 0) {
		ifr = alloca(ifc.ifc_len > 128 ? ifc.ifc_len : 128);
		ifc.ifc_req = ifr;
		if(ioctl(i, SIOCGIFCONF, &ifc) == 0)
			nifr = ifc.ifc_len / sizeof(struct ifreq);
	}
	close(i);

	int index = 0;
	for (i = 0; i < nifr; i++)
	{
		if (!strncmp(ifr[i].ifr_name, "lo", 2))
			continue;
		uint32_t ip = ((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr.s_addr;
		localip[index%64] = ip;
		index++;
	}
	return 0;
}

void report_err_2_nm (char *file, const char *func, int line, int ret)
{
	char val[256] = {0x0};
	snprintf(val, sizeof(val), "%s:%s:%d  ret=%d err %m", file, func, line, ret);
}

static int add_ip_2_pfs(char *sip, uint32_t uip, uint8_t role, void *ptr)
{
	uint32_t index = uip & IPMODE;
	list_head_t *mlist = &(allip[index]);
	t_ip_info *ip = malloc (sizeof(t_ip_info));
	if (ip == NULL)
	{
		LOG(glogfd, LOG_ERROR, "ip %s add malloc err %m\n", sip);
		return -1;
	}
	memset(ip, 0, sizeof(t_ip_info));
	INIT_LIST_HEAD(&(ip->hlist));
	snprintf(ip->sip, sizeof(ip->sip), "%s", sip);
	ip->uip = uip;
	ip->role = role;
	ip->ptr = ptr;
	list_add(&(ip->hlist), mlist);
	return 0;
}

int get_ip_info(uint32_t uip, t_ip_info **ipinfo)
{
	uint32_t index = uip & IPMODE;
	list_head_t *mlist = &(allip[index]);
	list_head_t *l;
	t_ip_info *ipinfo0;
	list_for_each_entry_safe_l(ipinfo0, l, mlist, hlist)
	{
		if (ipinfo0->uip == uip)
		{
			*ipinfo = ipinfo0;
			return 0;
		}
	}
	return -1;
}

int check_self_ip(uint32_t ip)
{
	int i = 0;
	for (i = 0; i < 64; i++)
	{
		if (localip[i] == 0)
			return -1;
		if (localip[i] == ip)
			return 0;
	}
	return -1;
}

void reload_config()
{
	g_config.max_data_connects = myconfig_get_intval("max_data_connects", 1024);
	g_config.max_task_run_once = myconfig_get_intval("max_task_run_once", 1024);
	g_config.poss_interval = myconfig_get_intval("poss_interval", 300);
	g_config.real_rm_time = myconfig_get_intval("real_rm_time", 7200);
	g_config.sync_dir_span = myconfig_get_intval("sync_dir_span", 300);
	g_config.check_task_timeout = myconfig_get_intval("check_task_timeout", 1);
	g_config.pfs_test = myconfig_get_intval("pfs_test", 0);
	g_config.task_timeout = myconfig_get_intval("task_timeout", 86400);
	g_config.sync_dir_count = myconfig_get_intval("sync_dir_count", 20);
	g_config.policy = myconfig_get_intval("task_policy", POLICY_DISKFREE)/POLICY_MAX;
	g_config.timeout = myconfig_get_intval("timeout", 3);
	LOG(glogfd, LOG_NORMAL, "task_timeout = %ld\n", g_config.task_timeout);
}

int init_global()
{
	srand(time(NULL));
	start_rand = rand();
	g_config.sig_port = myconfig_get_intval("sig_port", 39090);
	g_config.data_port = myconfig_get_intval("data_port", 49090);
	g_config.up_port = myconfig_get_intval("up_port", 59090);
	g_config.cktimeout = myconfig_get_intval("cktimeout", 5);
	g_config.lock_timeout = myconfig_get_intval("lock_timeout", 10);
	init_buff_size = myconfig_get_intval("socket_buff", 65536);
	if (init_buff_size < 20480)
		init_buff_size = 20480;

	g_config.retry = myconfig_get_intval("pfs_retry", 0) + 1;

	char *cmdnobody = "cat /etc/passwd |awk -F\\: '{if ($1 == \"nobody\") print $3\" \"$4}'";

	FILE *fp = popen(cmdnobody, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "execute  %s err %m\n", cmdnobody);
		return -1;
	}
	char buf[32] = {0x0};
	fgets(buf, sizeof(buf), fp);
	pclose(fp);
	char *t = strchr(buf, ' ');
	if (t == NULL)
	{
		fprintf(stderr, "execute  %s err %m [%s]\n", cmdnobody, buf);
		return -1;
	}
	g_config.dir_uid = atoi(buf);
	g_config.dir_gid = atoi(t+1);
	char *datadir = myconfig_get_value("pfs_datadir");
	if (datadir == NULL)
	{
		fprintf(stderr, "pfs_datadir is null\n");
		return -1;
	}
	snprintf(g_config.datadir, sizeof(g_config.datadir), "%s", datadir);
	DIR1 = myconfig_get_intval("dir_dir1", 100);
	DIR2 = myconfig_get_intval("dir_dir2", 100);

	reload_config();
	int i = 0;
	char *mindisk = myconfig_get_value("disk_minfree");
	if (mindisk == NULL)
		g_config.mindiskfree = 4 << 30;
	else
	{
		uint64_t unit_size = 1 << 30;
		char *t = mindisk + strlen(mindisk) - 1;
		if (*t == 'M' || *t == 'm')
			unit_size = 1 << 20;
		else if (*t == 'K' || *t == 'k')
			unit_size = 1 << 10;

		i = atoi(mindisk);
		if (i <= 0)
			g_config.mindiskfree = 4 << 30;
		else
			g_config.mindiskfree = i * unit_size;
	}

	char *v = myconfig_get_value("pfs_path");
	if (v == NULL)
		v = "/home/pfs/path";

	char path[256] = {0x0};
	for( i = 0; i < PATH_MAXDIR; i++)
	{
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "%s/%s", v, path_dirs[i]);
		if (access(path, R_OK|W_OK|X_OK) == 0)
			continue;
		if (mkdir(path, 0755))
		{
			LOG(glogfd, LOG_ERROR, "err mkdir %s %m\n", path);
			return -1;
		}
	}
	snprintf(g_config.path, sizeof(g_config.path), "%s", v);
	v = myconfig_get_value("pfs_sync_starttime");
	if (v == NULL)
		v = "01:00:00";
	snprintf(g_config.sync_stime, sizeof(g_config.sync_stime), "%s", v);
	v = myconfig_get_value("pfs_sync_endtime");
	if (v == NULL)
		v = "09:00:00";
	snprintf(g_config.sync_etime, sizeof(g_config.sync_etime), "%s", v);

	for( i = 0; i < IPLIST; i++)
	{
		INIT_LIST_HEAD(&(allip[i]));
	}
	INIT_LIST_HEAD(&alldomain);
	return 0;
}

static int init_poss()
{
	char *ip = myconfig_get_value("poss_server_ip");
	if (ip == NULL)
	{
		LOG(glogfd, LOG_ERROR, "no poss_server_ip!\n");
		return -1;
	}

	memset(possip, 0, sizeof(possip));
	get_uint32_ip(ip, possip);
	possport = myconfig_get_intval("poss_server_port", 49090);
	return add_ip_2_pfs(possip, str2ip(possip), ROLE_POSS_MASTER, NULL);
}

static int init_nameserver()
{
	memset(pfs_nameserver, 0, sizeof(pfs_nameserver));
	char* pval = NULL; int i = 0;
	for( i = 0; ( pval = myconfig_get_multivalue( "iplist_nameserver", i ) ) != NULL; i++ )
	{
		pfs_nameserver[i].uip = get_uint32_ip(pval, pfs_nameserver[i].ip);;
		if (i >= MAX_NAMESERVER)
		{
			LOG(glogfd, LOG_ERROR, "too many pfs_nameserver %d\n", MAX_NAMESERVER);
			break;
		}
		if (add_ip_2_pfs(pfs_nameserver[i].ip, pfs_nameserver[i].uip, ROLE_NAMESERVER, NULL))
			return -1;
	}
	if (i == 0)
	{
		LOG(glogfd, LOG_ERROR, "no iplist_nameserver!\n");
		return -1;
	}
	return 0;
}

static int add_storage_to_domain(char *ip, t_pfs_domain *g)
{
	if (g->ipcount >= MAX_IP_DOMAIN)
	{
		LOG(glogfd, LOG_ERROR, "too many ip more than %d in domain %s\n", MAX_IP_DOMAIN, g->domain); 
		return -1;
	}
	snprintf(g->sips[g->ipcount], 16, "%s", ip);
	g->ips[g->ipcount] = str2ip(ip);
	if (check_self_ip(g->ips[g->ipcount]) == 0)
		g->self[g->ipcount] = 1;
	if (add_ip_2_pfs(ip, g->ips[g->ipcount], ROLE_STORAGE, g))
		return -1;
	g->ipcount++;
	LOG(glogfd, LOG_NORMAL, "ip %s add in domain %s %d\n", ip, g->domain, g->ipcount); 
	return 0;
}

static int del_ip_from_pfs(uint32_t uip)
{
	uint32_t index = uip & IPMODE;
	list_head_t *mlist = &(allip[index]);
	list_head_t *l;
	t_ip_info *ip;
	list_for_each_entry_safe_l(ip, l, mlist, hlist)
	{
		if (ip->uip == uip)
		{
			list_del_init(&(ip->hlist));
			free(ip);
			return 0;
		}
	}

	return 0;
}

static void destroy_storage_ip(t_pfs_domain *d)
{
	int i = 0;
	for (; i < d->ipcount; i++)
	{
		LOG(glogfd, LOG_NORMAL, "del_ip %s\n", d->sips[i]);
		del_ip_from_pfs(d->ips[i]);
	}
}

static void destroy_old_group(char *gname)
{
	LOG(glogfd, LOG_NORMAL, "%s %s %d\n", ID, FUNC, LN);
	list_head_t *l;
	t_pfs_group_list *g;
	list_for_each_entry_safe_l(g, l, &grouplist, g_glist)
	{
		LOG(glogfd, LOG_NORMAL, "prepare do %s\n", g->group.group);
		if (strcmp(g->group.group, gname) == 0)
		{
			LOG(glogfd, LOG_NORMAL, "destroy_old_group %s\n", gname);
			list_del_init(&(g->g_glist));

			t_pfs_domain *d;
			list_head_t *l1;
			list_for_each_entry_safe_l(d, l1, &(g->group.g_dlist), d_glist)
			{
				list_del_init(&(d->d_glist));
				list_del_init(&(d->d_alist));
				destroy_storage_ip(d);
				free(d);
			}
			free(g);
		}
	}
	LOG(glogfd, LOG_NORMAL, "%s %s %d\n", ID, FUNC, LN);
}

static int init_group(t_pfs_group_list *g)
{
	destroy_old_group(g->group.group);
	char v[256] = {0x0};
	snprintf(v, sizeof(v), "iplist_%s", g->group.group);
	char *ips = myconfig_get_value(v);
	if (ips == NULL)
	{
		LOG(glogfd, LOG_ERROR, "no %s!\n", v);
		return -1;
	}
	INIT_LIST_HEAD(&(g->g_glist));
	list_add(&(g->g_glist), &grouplist);
	INIT_LIST_HEAD(&(g->group.g_dlist));
	
	FILE *fp = fopen(ips, "r");
	if (fp == NULL)
	{
		LOG(glogfd, LOG_ERROR, "open %s err %m\n", ips);
		return -1;
	}
	int ret = 0;
	char buf[2048] = {0x0};
	while (fgets(buf, sizeof(buf), fp))
	{
		char *t = strrchr(buf, '\n');
		if (t)
			*t = 0x0;
		t = strchr(buf, ':');
		if (t == NULL)
		{
			LOG(glogfd, LOG_ERROR, "err buf %s\n", buf);
			ret = -1;
			break;
		}
		*t = 0x0;
		t_pfs_domain *domain = malloc(sizeof(t_pfs_domain));
		if (domain == NULL)
		{
			LOG(glogfd, LOG_ERROR, "malloc err %m\n");
			ret = -1;
			break;
		}
		memset(domain, 0, sizeof(t_pfs_domain));
		INIT_LIST_HEAD(&(domain->d_glist));
		INIT_LIST_HEAD(&(domain->d_alist));
		list_add(&(domain->d_glist), &(g->group.g_dlist));
		list_add(&(domain->d_alist), &alldomain);
		snprintf(domain->domain, sizeof(domain->domain), "%s", buf);

		char *s = t+1;
		while (1)
		{
			t = strchr(s, ',');
			if (t)
			{
				*t = 0x0;
				if(add_storage_to_domain(s, domain))
					return -1;
				s = t + 1;
			}
			else
				break;
		}
		if (add_storage_to_domain(s, domain))
		{
			ret = -1;
			break;
		}
	}
	fclose(fp);
	return ret;
}

int init_storage()
{
	if (get_cfg_lock())
	{
		LOG(glogfd, LOG_ERROR, "get_cfg_lock err ,exit!\n");
		stop = 1;
		return -1;
	}
	int ret = 0;
	char* pval = NULL; int i = 0;
	for( i = 0; ( pval = myconfig_get_multivalue( "group_name", i ) ) != NULL; i++ )
	{
		t_pfs_group_list *group = (t_pfs_group_list *)malloc (sizeof(t_pfs_group_list));
		if (group == NULL)
		{
			LOG(glogfd, LOG_ERROR, "malloc err %m\n");
			ret = -1;
			break;
		}
		memset(group, 0, sizeof(t_pfs_group_list));
		snprintf(group->group.group, sizeof(group->group.group), "%s", pval);
		if (init_group(group))
		{
			LOG(glogfd, LOG_ERROR, "init_group %s err %m\n", pval);
			ret = -1;
			break;
		}
	}
	if (i == 0)
	{
		LOG(glogfd, LOG_ERROR, "no domain_name!\n");
		ret = -1;
	}
	release_cfg_lock();
	return ret;
}

static int set_info_disk(char *outdir, int d1, int d2)
{
	struct statfs sfs;
	struct stat diskstat;
	int err = 0;
	if (lstat(outdir, &diskstat))
	{
		LOG(glogfd, LOG_ERROR, "stat %s err %m\n", outdir);
		err = 1;
	}
	if (statfs(outdir, &sfs))
	{
		LOG(glogfd, LOG_ERROR, "statfs %s err %m\n", outdir);
		err = 1;
	}

	t_disk_info *disk_info = &diskinfo[0];
	int i = 0;
	for (; i < MAXDISKS; i++)
	{
		if (disk_info->count == 0)
		{
			memcpy(&(disk_info->fsid), &(sfs.f_fsid), sizeof(sfs.f_fsid));
			disk_info->pathcluster[0] = d1*DIR1 + d2;
			disk_info->count = 1;
			snprintf(disk_info->firstpath, sizeof(disk_info->firstpath), "%s", outdir);
			break;
		}
		if (memcmp(&(disk_info->fsid), &(sfs.f_fsid), sizeof(sfs.f_fsid)) == 0)
		{
			if (disk_info->count >= TOTALDIRS)
			{
				LOG(glogfd, LOG_ERROR, " too many dir in %s \n", outdir);
				return -1;
			}
			int dirs = d1*DIR1 + d2;
			int j = 0;
			for (; j < TOTALDIRS; j++)
			{
				if (disk_info->pathcluster[j] == dirs)
					goto set_end;
				if (disk_info->pathcluster[j] == 0)
				{
					disk_info->pathcluster[j] = dirs;
					disk_info->count++;
					goto set_end;
				}
			}
			LOG(glogfd, LOG_ERROR, " any wrong in %s \n", outdir);
			return -1;
		}
		disk_info++;
	}
set_end:
	if (err == 0)
		max_disk = disk_info;
	return 0;
}

int refresh_disk_info(char *args)
{
	char outdir[256] = {0x0};
	int d1 = 0;
	int d2 = 0;
	for ( d1 = 0; d1 < DIR1; d1++)
	{
		for (d2 = 0; d2 < DIR2; d2++)
		{
			sprintf(outdir, "%s/%d/%d/", g_config.datadir, d1, d2);
			if (set_info_disk(outdir, d1, d2))
			{
				LOG(glogfd, LOG_ERROR, "set_info_disk %s err %m\n", outdir);
				return -1;
			}
		}
	}
	if (max_disk == NULL)
	{
		LOG(glogfd, LOG_ERROR, "max_disk is null\n");
		return -1;
	}
	LOG(glogfd, LOG_NORMAL, "max_disk count %u:%u\n", max_disk->count, max_disk->index);
	return 0;
}

static void set_self_disk_err(int k, int v)
{
	int j;
	for (j =0; j < self_domain->ipcount; j++)
	{
		if (self_domain->self[j] == 1)
		{
			self_domain->iperrdisk[j][k%MAXDISKS] = v;
			return;
		}
	}
	LOG(glogfd, LOG_ERROR, "set_self_disk_err ERR %d %d\n", k, v);
}

static int check_domain_disk_err(int k)
{
	int j;
	for (j =0; j < self_domain->ipcount; j++)
	{
		if (self_domain->iperrdisk[j][k%MAXDISKS])
			return 1;
	}
	return 0;
}

void refresh_maxfreedisk(char *args)
{
	srand(time(NULL));
	t_disk_info *disk_info = &diskinfo[0];
	uint32_t curmaxvalue = 0;
	int index = 0;
	int i = 0;
	for (; i < MAXDISKS; i++)
	{
		if (disk_info->count == 0)
			break;

		struct stat diskstat;
		if (lstat(disk_info->firstpath, &diskstat))
		{
			LOG(glogfd, LOG_ERROR, "stat %s err %m\n", disk_info->firstpath);
			set_self_disk_err(i, 1);
			disk_info++;
			continue;
		}

		struct statfs sfs;
		if (statfs(disk_info->firstpath, &sfs))
		{
			LOG(glogfd, LOG_ERROR, "statfs %s err %m\n", disk_info->firstpath);
			set_self_disk_err(i, 1);
			disk_info++;
			continue;
		}

		if (g_config.pfs_test == 1)
		{
			if (rand()%2)
			{
				LOG(glogfd, LOG_ERROR, "pfs_test open , set rand disk err [%d]!\n", i);
				set_self_disk_err(i, 1);
				disk_info++;
				continue;
			}
		}

		set_self_disk_err(i, 0);
		if (check_domain_disk_err(i))
		{
			LOG(glogfd, LOG_ERROR, "domain %s err\n", disk_info->firstpath);
			disk_info++;
			continue;
		}

		LOG(glogfd, LOG_NORMAL, "statfs %s\n", disk_info->firstpath);
		disk_info++;
		uint64_t diskfree = (sfs.f_bfree * sfs.f_bsize) >> 30;
		if (curmaxvalue < diskfree)
		{
			curmaxvalue = diskfree;
			index = i;
		}
	}

	maxfreevalue = curmaxvalue;
	max_disk = &diskinfo[index];
}

int pfs_init()
{
	return init_local_ip() || init_nameserver () || init_poss() || init_storage() || init_file_filter();
}

int init_storage_dir()
{
	maxfreevalue = 0;
	memset(&diskinfo, 0, sizeof(diskinfo));
	char outdir[256] = {0x0};
	int d1 = 0;
	int d2 = 0;
	for ( d1 = 0; d1 < DIR1; d1++)
	{
		for (d2 = 0; d2 < DIR2; d2++)
		{
			sprintf(outdir, "%s/%d/%d/", g_config.datadir, d1, d2);
			if (createdir(outdir))
			{
				LOG(glogfd, LOG_ERROR, "createdir %s err %m\n", outdir);
				if (g_config.pfs_test == 0)
					return -1;
			}
		}
	}
	if(refresh_disk_info(NULL))
		return -1;
	refresh_maxfreedisk(NULL);
	t_pfs_timer pfs_timer;
	memset(&pfs_timer, 0, sizeof(pfs_timer));
	pfs_timer.loop = 1;
	pfs_timer.span_time = 600;
	pfs_timer.cb = refresh_maxfreedisk;
	if (add_to_delay_task(&pfs_timer))
	{
		LOG(glogfd, LOG_ERROR, "add_to_delay_task err %m\n");
		return -1;
	}
	return 0;
}

int get_self_role_ip_domain(uint32_t *ip, t_pfs_domain *domains)
{
	int i = 0;
	t_ip_info *ipinfo;
	for (i = 0; i < 64; i++)
	{
		if (localip[i] == 0)
			return ROLE_UNKOWN;
		if (get_ip_info(localip[i], &ipinfo) == 0)
		{
			*ip = localip[i];
			if (ipinfo->role == ROLE_STORAGE)
				memcpy(domains, ipinfo->ptr, sizeof(t_pfs_domain));
			return ipinfo->role;
		}
	}
	return ROLE_UNKOWN;
}

int get_dir1_dir2(char *pathsrc, int *dir1, int *dir2)
{
	char path[256] = {0x0};
	snprintf(path, sizeof(path), "%s", pathsrc);
	char *s = path;
	char *t = strrchr(path, '/');
	if (t == NULL)
	{
		LOG(glogfd, LOG_ERROR, "error path %s\n", pathsrc);
		return -1;
	}
	*t = 0x0;
	t = strrchr(s, '/');
	if (t == NULL)
	{
		LOG(glogfd, LOG_ERROR, "error path %s\n", pathsrc);
		return -1;
	}
	*t = 0x0;
	t++;
	*dir2 = atoi(t)%DIR2;

	t = strrchr(s, '/');
	if (t == NULL)
	{
		LOG(glogfd, LOG_ERROR, "error path %s\n", pathsrc);
		return -1;
	}
	t++;
	*dir1 = atoi(t)%DIR1;
	LOG(glogfd, LOG_TRACE, "path %s get %d %d\n", path, *dir1, *dir2);
	return 0;
}

int init_pfs_thread(t_thread_arg *arg)
{
	int iret = 0;
	if((iret = register_thread(arg->name, pfs_signalling_thread, (void *)arg)) < 0)
		return iret;
	LOG(glogfd, LOG_DEBUG, "%s:%s:%d\n", ID, FUNC, LN);
	return 0;
}

void update_domain_info(uint32_t taskcount, uint32_t load, t_pfs_domain *domain)
{
	domain->taskcount += taskcount;
	domain->load += load;
}

int get_self_err_disk(uint8_t **errdisk)
{
	int j;
	for (j =0; j < self_domain->ipcount; j++)
	{
		if (self_domain->self[j] == 1)
		{
			*errdisk = self_domain->iperrdisk[j];
			return 0;
		}
	}
	LOG(glogfd, LOG_ERROR, "get_self_err_disk err!\n");
	return -1;
}

void set_domain_diskerr(char *errdisk, uint32_t ip)
{
	int j;
	for (j =0; j < self_domain->ipcount; j++)
	{
		if (self_domain->ips[j] == ip)
		{
			memcpy(self_domain->iperrdisk[j], errdisk, MAXDISKS);
			return ;
		}
	}
	LOG(glogfd, LOG_ERROR, "set_domain_diskerr %u!\n", ip);
}

int get_domain(char *domain, t_pfs_domain *domains)
{
	list_head_t *l;
	t_pfs_domain *domains0;
	list_for_each_entry_safe_l(domains0, l, &alldomain, d_alist)
	{
		if (strcmp(domain, domains0->domain) == 0)
		{
			domains0->index++;
			if (domains0->index >= domains0->ipcount)
				domains0->index = 0;
			LOG(glogfd, LOG_NORMAL, "domain %s index %d ipcount %d\n", domain, domains0->index, domains0->ipcount);
			memcpy(domains, domains0, sizeof(t_pfs_domain));
			return 0;
		}
	}
	return -1;
}

inline void get_max_free_storage(int *d1, int *d2)
{
	if (max_disk == NULL)
		return;
	LOG(glogfd, LOG_NORMAL, "get max_disk count %u:%u\n", max_disk->count, max_disk->index);
	uint32_t maxfreedirs = max_disk->pathcluster[max_disk->index];
	*d1 = maxfreedirs / DIR1;
	*d2 = maxfreedirs % DIR1;
	max_disk->index++;
	if (max_disk->index >= max_disk->count)
		max_disk->index = 0;
}

int get_cfg_lock()
{
	int ret = pthread_mutex_lock(&cfgmutex);
	if (ret != 0)
	{
		if (ret != EDEADLK)
		{
			LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_lock error %d\n", FUNC, LN, ret);
			report_err_2_nm(ID, FUNC, LN, ret);
			return -1;
		}
	}
	return 0;
}

int release_cfg_lock()
{
	if (pthread_mutex_unlock(&cfgmutex))
		LOG(glogfd, LOG_ERROR, "ERR %s:%d pthread_mutex_unlock error %m\n", FUNC, LN);
	return 0;
}


