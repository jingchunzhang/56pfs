/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef _56_PFS_INIT_H_
#define _56_PFS_INIT_H_
#include "nm_app_pfs.h"
#include "list.h"
#include "pfs_so.h"
#include "global.h"
#include "util.h"
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ALLMASK 0xFF
extern int glogfd;
extern int init_buff_size ;
extern int DIR1 ;
extern int DIR2 ;
#define TOTALDIRS 10000

enum {PATH_INDIR = 0, PATH_OUTDIR, PATH_WKDIR, PATH_BKDIR, PATH_TMPDIR, PATH_MAXDIR};
extern const char *path_dirs[];
extern const char *name_policy[]; 

enum {POLICY_DISKFREE = 0, POLICY_LOAD, POLICY_TASKCOUNT, POLICY_MAX};

typedef struct {
	uint8_t role;
	char sip[16];
	uint32_t uip;
	void * ptr;  /*while role = STORAGE, ptr point as it's domain*/
	list_head_t hlist;
} t_ip_info;

typedef struct {
	char domain[128];
	char sips[MAX_IP_DOMAIN][16];
	char self[MAX_IP_DOMAIN];
	char offline[MAX_IP_DOMAIN];
	uint8_t iperrdisk[MAX_IP_DOMAIN][MAXDISKS];
	uint32_t ips[MAX_IP_DOMAIN];
	uint32_t diskfree;
	uint32_t taskcount;
	uint32_t load;
	int ipcount;
	int index;
	list_head_t d_glist;
	list_head_t d_alist;
} t_pfs_domain;

typedef struct {
	char group[128];
	list_head_t g_dlist;   //domain
} t_pfs_group;

typedef struct {
	t_pfs_group group;
	list_head_t g_glist;
} t_pfs_group_list;

typedef struct {
	fsid_t fsid; //fs id
	char firstpath[256];
	int count;
	int index;
	uint32_t pathcluster[TOTALDIRS];
} t_disk_info;

typedef struct {
	char ip[16];
	uint32_t uip;
} t_pfs_nameserver;

typedef struct {
	uint16_t sig_port;
	uint16_t data_port;
	uint16_t up_port;
	uint16_t timeout;
	uint16_t cktimeout;
	uint16_t bk;
	time_t task_timeout;
	time_t real_rm_time;
	uint64_t mindiskfree;
	char path[256];
	char datadir[256];
	char sync_stime[12];
	char sync_etime[12];
	uint16_t max_data_connects;
	uint16_t max_task_run_once;
	uint16_t dir_gid;
	uint16_t dir_uid;
	uint16_t lock_timeout;
	uint16_t poss_interval;
	uint16_t sync_dir_span;
	uint8_t pfs_test;
	uint8_t retry;
	uint8_t policy;
	uint8_t check_task_timeout;
	uint8_t sync_dir_count;
} t_g_config;

enum {IP_ONLINE = 0, IP_OFFLINE};

extern t_g_config g_config;

int pfs_init();

int init_global();

int get_self_role_ip_domain(uint32_t *ip, t_pfs_domain *domains);

int init_pfs_thread(t_thread_arg *name);

void fini_pfs_thread(char *threadname);

void report_err_2_nm (char *file, const char *func, int line, int ret);

void reload_config();

int get_dir1_dir2(char *pathsrc, int *dir1, int *dir2);

int init_storage_dir();

#define GET_MAX_FREEVALUE()	\
do {	\
	return maxfreevalue;	\
}while(0)

inline void get_max_free_storage(int *d1, int *d2);

int set_domain_by_uip(t_pfs_domain **domains, uint32_t ip);

int get_domain(char *domain, t_pfs_domain *domains);

void update_domain_info(uint32_t taskcount, uint32_t load, t_pfs_domain *domain);

int get_self_err_disk(uint8_t **errdisk);

void set_domain_diskerr(char *errdisk, uint32_t ip);

int init_storage();

int get_cfg_lock();

int release_cfg_lock();

#endif
