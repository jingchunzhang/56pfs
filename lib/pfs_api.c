/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_api.h"
#include "common.h"
#include "mybuff.h"
#include "myepoll.h"
#include "protocol.h"
#include "util.h"
#include "pfs_task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sendfile.h>
#include <signal.h>

#define SOCK_SWITCH 0x01  /* check_req return this, mean to change stat!*/
#define SOCK_DATA_OVER 0x02  /* client send data ok*/
#define SOCK_CLOSE 0x10 /* peer close or active close */
#define SOCK_SEND  0x20 /* more data need send at next loop */
#define SOCK_SELECT_ERR 0x30 /* select error */
#define SOCK_COMP  0x40 /*socker data send over */
#define SOCK_SELECT_TO 0x60 /* select time out*/
#define SOCK_RECV  0x80 /*socker recv some data */
#define GSIZE 1073741824    /*sendfile max send once*/

static __thread int client_stat;
static __thread int ns_cache_flag = 0;
static __thread t_api_cache ns_cache;
char *pfserrmsg[] = {"OK", "", "", "PFS_CON_NAMESERVER", "PFS_CON_STORAGE", "PFS_LOCAL_FILE", "PFS_SERVER_ERR", "PFS_ERR_TYPE", "PFS_ERR_INFO_UNCOM", "PFS_REALPATH", "PFS_STORAGE_FILE"};

enum CLIENT_STAT {GET_STORAGE = 0, PUT_DATA};

static int do_recv(int fd, struct mybuff *recv_buff)
{
	char iobuf[2048];
	int n = 0;
	int total = 0;
	while (1)
	{
		n = recv(fd, iobuf, sizeof(iobuf), MSG_DONTWAIT);
		if (n > 0)
		{
			mybuff_setdata(recv_buff, iobuf, n);
			total += n;
			if (n == sizeof(iobuf))
				continue;
			return total;
		}
		if (n == 0)
			return -1;
		if (errno == EINTR)
			continue;
		if (errno == EAGAIN)
			return 0;
		return -1;
	}
	return -1;
}

static int do_send(int fd, struct mybuff *send_buff)
{
	size_t n = 0;
	int lfd;
	off_t start;
	char* data;
	size_t len;
	if(!mybuff_getdata(send_buff, &data, &len)) 
	{
		while (1)
		{
			n = send(fd, data, len, MSG_DONTWAIT | MSG_NOSIGNAL);
			if(n > 0) 
			{
				mybuff_skipdata(send_buff, n);
				if (n < len)
					return SOCK_SEND;
				break;
			}
			else if(errno == EINTR) 
				continue;
			else if(errno == EAGAIN) 
				return SOCK_SEND;
			else 
				return SOCK_CLOSE;
		}
	}
	if(!mybuff_getfile(send_buff, &lfd, &start, &len))
	{
		size_t len1 = len > GSIZE ? GSIZE : len;
		while (1)
		{
			n = sendfile64(fd, lfd, &start, len1);
			if(n > 0) 
			{
				mybuff_skipfile(send_buff, n);
				if(n < len) 
					return SOCK_SEND;
				return SOCK_COMP;
			}
			else if(errno == EINTR) 
				continue;
			else if(errno == EAGAIN)
				return SOCK_SEND;
			else 
				return SOCK_CLOSE;
		}
	}
	return SOCK_COMP;
}

int check_req(struct mybuff *recv_buff, char *result)
{
	t_pfs_sig_head h;
	t_pfs_sig_body b;
	memset(&h, 0, sizeof(h));
	memset(&b, 0, sizeof(b));
	char *data;
	size_t datalen;
	if (mybuff_getdata(recv_buff, &data, &datalen))
		return SOCK_RECV;
	int ret = parse_sig_msg(&h, &b, data, datalen);
	if(ret > 0)
		return SOCK_RECV;
	if (ret == E_PACKET_ERR_CLOSE)	
		return SOCK_CLOSE;
	memcpy(result, b.body, h.bodylen);
	if (client_stat == GET_STORAGE)
		return SOCK_SWITCH;
	return SOCK_DATA_OVER;
}

int connect_storage(char *s, char *errmsg)
{
	char *t = strchr(s, ':');
	if (t == NULL)
	{
		sprintf(errmsg, "errmsg:%s", s);
		return -1;
	}
	*t = 0x0;
	int fd = createsocket(s, atoi(t+1));
	if (fd >= 0 && ns_cache_flag && ns_cache.port == 0)
	{
		memset(&ns_cache, 0, sizeof(ns_cache));
		ns_cache.uptime = time(NULL);
		snprintf(ns_cache.ip, sizeof(ns_cache.ip), "%s", s);
		ns_cache.port = atoi(t+1);
		ns_cache.fd = -1;
	}
	return fd;
}

static int get_localfile_stat(t_task_base *task)
{
	struct stat filestat;
	if (stat(task->filename, &filestat))
		return -1;
	task->mtime = filestat.st_mtime;
	task->fsize = filestat.st_size;
	if (getfilemd5(task->filename, task->filemd5))
		return -1;
	return 0;
}

static int create_req_msg(struct mybuff *send_buff, struct mybuff *recv_buff, t_api_info *api)
{
	mybuff_reinit(send_buff);
	mybuff_reinit(recv_buff);
	char obuf[2048] = {0x0};
	int bodylen = 0;
	t_pfs_sig_body ob;;
	memset(&ob, 0, sizeof(ob));
	bodylen = snprintf(ob.body, sizeof(ob.body), "%s", api->group);
	int n = 0;
	if (client_stat == GET_STORAGE)
	{
		if (api->type == TASK_ADDFILE)
			n = create_sig_msg(UPLOAD_REQ, api->nametype, &ob, obuf, bodylen);
		else
			n = create_sig_msg(MODI_DEL_REQ, api->addtype, &ob, obuf, bodylen);
		mybuff_setdata(send_buff, obuf, n);
		return 0;
	}
	t_task_base base;
	memset(&base, 0, sizeof(base));
	base.type = api->type;
	if (base.type == TASK_DELFILE)
		snprintf(base.filename, sizeof(base.filename), "%s", api->remotefile);
	else
	{
		snprintf(base.filename, sizeof(base.filename), "%s", api->localfile);
		if (get_localfile_stat(&base))
			return -1;
		if (base.fsize == 0)
			return -1;
		if (base.type == TASK_MODYFILE)
		{
			memset(base.filename, 0, sizeof(base.filename));
			snprintf(base.filename, sizeof(base.filename), "/%s", api->remotefile);
			api->addtype = ADD_PATH_BY_UPLOAD;
		}

		if (api->addtype == ADD_DIR_BY_UPLOAD)
		{
			memset(base.filename, 0, sizeof(base.filename));
			snprintf(base.filename, sizeof(base.filename), "%s", api->remotefile);
		}
	}
	n = create_sig_msg(CMD_PUT_FILE_REQ, api->addtype, (t_pfs_sig_body *)&base, obuf, sizeof(base));
	mybuff_setdata(send_buff, obuf, n);
	if (base.type == TASK_DELFILE)
		return 0;
	int fd = open(api->localfile, O_RDONLY);
	if (fd < 0)
		return -1;
	mybuff_setfile(send_buff, fd, 0, base.fsize);
	return 0;
}

static int check_rsp_from_storage(t_api_info *api)
{
	if (strncmp(api->errmsg, "OK|", 3))
		return -1;
	char *s = api->errmsg + 3;
	char *e = strchr(s, '|');
	if (e == NULL)
		return -1;
	*e = 0x0;
	char *t = s;
	while (1)
	{
		if (*t == '/')
			t++;
		else
		{
			t--;
			break;
		}
	}
	strcpy(api->remotefile, t);
	*e = '|';
	memset(api->group, 0, sizeof(api->group));
	strcpy(api->group, e + 1);
	return 0;
}

static int upload_file_inter(char *nameip, int nameport, t_api_info *api)
{
	client_stat = GET_STORAGE;
	int fd = -1;
	if (ns_cache_flag && ns_cache.port > 0)
	{
		if (ns_cache.fd > 0)
		{
			fd = ns_cache.fd;
			client_stat = PUT_DATA;
		}
		else
		{
			fd = createsocket(ns_cache.ip, ns_cache.port);
			if (fd < 0)
				fprintf(stderr, "connect %s:%d err %m\n", ns_cache.ip, ns_cache.port);
			else
				client_stat = PUT_DATA;
		}
	}
	if (fd < 0)
	{
		fd = createsocket(nameip, nameport);
		if (fd < 0)
		{
			fprintf(stderr, "connect %s:%d err %m\n", nameip, nameport);
			return PFS_CON_NAMESERVER;
		}
	}
	if (api->addtype == ADD_PATH_BY_UPLOAD &&(api->type == TASK_ADDFILE || TASK_MODYFILE == api->type))
	{
		/*
		char abspath[256] = {0x0};
		if (realpath(api->localfile, abspath) == NULL)
		{
			fprintf(stderr, "realpath %s err %m\n", api->localfile);
			close(fd);
			return PFS_REALPATH;
		}
		memset(api->localfile, 0, sizeof(api->localfile));
		snprintf(api->localfile, sizeof(api->localfile), "%s", abspath);
		*/
	}
	
	fcntl(fd, F_SETFL, O_RDWR|O_NONBLOCK);
	char *errmsg = api->errmsg;
	struct mybuff send_buff;
	struct mybuff recv_buff;
	mybuff_init(&send_buff);
	mybuff_init(&recv_buff);
	char result[2048] = {0x0};
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	int ret = 0;
	if(create_req_msg(&send_buff, &recv_buff, api))
	{
		ret = PFS_LOCAL_FILE;
		goto up_err;
	}
	int havedata = 1;
	while (1)
	{
		FD_ZERO( &rfds );
		FD_ZERO( &wfds );
		FD_SET( fd, &rfds );
		if (havedata)
			FD_SET( fd, &wfds );
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select( fd + 1, &rfds, &wfds, NULL, &tv);
		if (ret  == -1)
		{
			fprintf(stderr, "select %d err %m\n", fd);
			ret = SOCK_SELECT_ERR;
			break;
		}

		if (ret == 0)
		{
			ret = SOCK_SELECT_TO;
			snprintf(api->errmsg, sizeof(api->errmsg), "time out %d!", client_stat);
			break;
		}

		if( FD_ISSET( fd, &wfds ) )
		{
			havedata = 0;
			ret = do_send(fd, &send_buff);
			if (ret == SOCK_CLOSE)
				break;
			else if (ret == SOCK_SEND)
				havedata = 1;
		}

		if( FD_ISSET( fd, &rfds ) )
		{
			ret = do_recv(fd, &recv_buff);
			if (ret < 0)
				break;
			else if (ret == 0)
				continue;
			ret = check_req(&recv_buff, result);
			if (ret == SOCK_SWITCH)
			{
				close(fd);
				client_stat = PUT_DATA;
				if (create_req_msg(&send_buff, &recv_buff, api))
				{
					ret = PFS_LOCAL_FILE;
					goto up_err;
				}
				fd = connect_storage(result, errmsg);
				if(fd < 0)
				{
					snprintf(api->errmsg, 256, "%s", result);
					ret = PFS_CON_STORAGE;
					goto up_err;
				}
				fcntl(fd, F_SETFL, O_RDWR|O_NONBLOCK);
				havedata = 1;
				continue;
			}
			else if ( ret == SOCK_DATA_OVER)
			{
				snprintf(api->errmsg, 256, "%s", result);
				break;
			}
			else if ( ret == SOCK_CLOSE)
				break;
		}
	}
up_err:
	if (fd > 0)
		close(fd);
	mybuff_fini(&send_buff);
	mybuff_fini(&recv_buff);
	if (ret != SOCK_DATA_OVER)
		return ret;
	if (check_rsp_from_storage(api))
		return PFS_STORAGE_FILE;
	return PFS_OK;
}

int operate_pfs(char *nameip, int nameport, t_api_info *api)
{
	signal(SIGPIPE, SIG_IGN);
	if (api->type > TASK_MODYFILE || api->type < TASK_ADDFILE)
		return PFS_ERR_TYPE;
	if (api->type == TASK_ADDFILE)
	{
		if (strlen(api->localfile) == 0)
			return PFS_ERR_INFO_UNCOM;
	}
	else
	{
		if (strlen(api->remotefile) == 0 || strlen(api->group) == 0)
			return PFS_ERR_INFO_UNCOM;
	}
	return upload_file_inter(nameip, nameport, api);
}

void init_ns_cache()
{
	memset(&ns_cache, 0, sizeof(ns_cache));
	ns_cache_flag = 1;
}

