/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef _PFS_TASK_H_
#define _PFS_TASK_H_

/*
 */

#include "list.h"
#include "pfs_api.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

enum {OPER_IDLE, OPER_GET_REQ, OPER_GET_RSP, OPER_SYNC_2_DOMAIN, OPER_PUT_REQ, OPER_PUT_RSP};  

enum {TASK_Q_DELAY = 0, TASK_Q_WAIT, TASK_Q_WAIT_TMP, TASK_Q_WAIT_SYNC, TASK_Q_WAIT_SYNC_IP, TASK_Q_WAIT_SYNC_DIR, TASK_Q_WAIT_SYNC_DIR_TMP, TASK_Q_SYNC_DIR, TASK_Q_SYNC_DIR_TMP, TASK_Q_RUN, TASK_Q_FIN, TASK_Q_CLEAN, TASK_Q_HOME, TASK_Q_SEND, TASK_Q_RECV, TASK_Q_SYNC_POSS, TASK_Q_SYNC_DIR_REQ, TASK_Q_SYNC_DIR_RSP, TASK_Q_UNKNOWN}; /*任务队列*/  

enum {OVER_UNKNOWN = 0, OVER_OK, OVER_E_MD5, OVER_PEERERR, TASK_EXIST, OVER_PEERCLOSE, OVER_UNLINK, OVER_TIMEOUT, OVER_MALLOC, OVER_SRC_DOMAIN_ERR, OVER_SRC_IP_OFFLINE, OVER_E_OPEN_SRCFILE, OVER_E_OPEN_DSTFILE, OVER_E_IP, OVER_E_TYPE, OVER_SEND_LEN, OVER_TOO_MANY_TRY, OVER_DISK_ERR, OVER_LAST};  /*任务结束时的状态*/

enum {GET_TASK_Q_ERR = -1, GET_TASK_Q_OK, GET_TASK_Q_NOTHING};  /*从指定队列取任务的结果*/

enum {TASK_DST = 0, TASK_SOURCE, TASK_SRC_NOSYNC, TASK_SYNC_ISDIR, TASK_SYNC_VOSS_FILE}; /*本次任务是否需要相同组机器同步 */

extern const char *task_status[TASK_Q_UNKNOWN]; 
extern const char *over_status[OVER_LAST]; 
typedef struct {
	char filename[256];
	char tmpfile[256];
	char retfile[256];
	unsigned char filemd5[17];  /*change md5 for more efficiency*/
	uint32_t dstip;    /*本次任务目标ip，在GET或者PUT_FROM时，此ip是本机ip*/
	time_t starttime;
	time_t mtime;
	off_t fsize;
	uint8_t type;         /*文件变换类型，删除还是新增*/
	uint8_t retry;     /*任务执行失败时，根据配置是否执行重新发起任务，已经重试次数，不能超过设定重试次数*/
	uint8_t overstatus; /*结束状态*/
	uint8_t bk;
}t_task_base;

typedef struct {
	off_t processlen; /*需要获取或者发送的数据长度*/
	off_t lastlen; /*上一个周期 处理的长度，初始化为0 */
	time_t   lasttime; /*上个周期时间戳*/
	time_t   starttime; /*开始时间戳*/
	time_t	 endtime; /*结束时间戳*/
	char peerip[16];  /*对端ip*/
	uint8_t oper_type; /*是从FCS GET文件，还是向同组CS  GET文件 */
	uint8_t need_sync; /*TASK_SOURCE：本次任务源头，TASK_DST：本次任务目的之一 */
	uint8_t sync_dir;  /**/
	uint8_t bk;
}t_task_sub;

typedef struct {
	t_task_base base;
	t_task_sub  sub;
	void *user;
}t_pfs_taskinfo;

typedef struct {
	t_pfs_taskinfo task;
	list_head_t llist;
	list_head_t userlist;
	time_t last;
	uint8_t status;
	uint8_t flag;
	uint8_t bk[2];
} t_pfs_tasklist;

typedef struct {
	int d1;
	int d2;
	time_t starttime;
	time_t endtime; 
	char type;  
	char bk[3];
} t_pfs_sync_task;

typedef struct {
	t_pfs_sync_task sync_task;
	list_head_t list;
} t_pfs_sync_list;

typedef void (*timeout_task)(t_pfs_tasklist *task);

int pfs_get_task(t_pfs_tasklist **task, int status);

int pfs_set_task(t_pfs_tasklist *task, int status);

int init_task_info();

int scan_some_status_task(int status, timeout_task cb);

int get_task_count(int status);

void do_timeout_task();

void report_2_nm();

void dump_task_info (int logfd, char *from, int line, t_task_base *task);

void do_timeout_task();

#endif
