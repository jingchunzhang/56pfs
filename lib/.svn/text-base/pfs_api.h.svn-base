/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef __PFS_API_H_
#define __PFS_API_H_
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <unistd.h>
enum {ADD_PATH_BY_STORAGE = 0, ADD_PATH_BY_UPLOAD, ADD_PATH_BY_BNAME, ADD_DIR_BY_UPLOAD = 0x04, ADD_QUERY_DIR, ADD_QUERY_USR1, ADD_QUERY_USR2};
enum {ADD_SET_GROUP = 0, ADD_SET_DOMAIN};
typedef struct {
	char errmsg[256];  /*操作不成功时，此字段包含具体的错误信息*/
	char localfile[256]; /*本地文件名*/
	char remotefile[256]; /*存储在远端的文件名，绝对路径*/
	char group[128];  /*存储在远端的分组*/
	uint8_t type;  /* 操作类型， TASK_ADDFILE(新增文件), TASK_DELFILE(删除文件), TASK_MODYFILE(修改覆盖文件) */
	uint8_t addtype; /*新增文件时，路径指定标志：ADD_PATH_BY_STORAGE 由存储端指定，ADD_PATH_BY_UPLOAD，上传者指定*/
	uint8_t nametype; /*当group有值时，nametype=ADD_SET_GROUP 指定组名，nametype=ADD_SET_DOMAIN指定域名*/
} t_api_info;

typedef struct {
	time_t uptime;
	char ip[16];
	int port;
	int fd; /*storage fd*/
} t_api_cache;

int operate_pfs(char *nameip, int nameport, t_api_info *api);

void init_ns_cache();

enum {TASK_ADDFILE = '0', TASK_DELFILE, TASK_MODYFILE, TASK_SYNCDIR};  /* 任务类型 */

extern char *pfserrmsg[]; 
enum PFS_RET {PFS_OK = 0, PFS_CON_NAMESERVER = 3, PFS_CON_STORAGE, PFS_LOCAL_FILE, PFS_SERVER_ERR, PFS_ERR_TYPE, PFS_ERR_INFO_UNCOM, PFS_REALPATH, PFS_STORAGE_FILE};

#endif
