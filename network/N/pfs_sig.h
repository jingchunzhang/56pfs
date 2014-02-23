/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef __PFS_SIG_SO_H
#define __PFS_SIG_SO_H
#include "list.h"
#include "pfs_file_filter.h"
#include "global.h"
#include "pfs_init.h"
#include "pfs_time_stamp.h"
#include "common.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>

#define SELF_ROLE ROLE_FCS

enum SOCK_STAT {LOGOUT = 0, CONNECTED, LOGIN, HB_SEND, HB_RSP, IDLE, RECV_LAST, SEND_LAST};

typedef struct {
	list_head_t alist;
	list_head_t hlist;
	int fd;
	uint32_t hbtime;
	uint32_t ip;
	uint32_t taskcount;
	uint32_t load;
	uint32_t diskfree;
	uint8_t server_stat; /* server stat*/
	uint8_t close;
} pfs_fcs_peer;

typedef struct {
	int flag;   /*0: idle, 1: sync_dir , 2: no more task for sync*/
	int total_sync_task;
	int total_synced_task;
} t_sync_para;

typedef struct {
	char olddomain[64];
	char newdomain[64];
	list_head_t hlist;
} t_domain_change;

typedef struct {
	char domain[64];
	int type;
	list_head_t hlist;
} t_storage_select;
#endif
