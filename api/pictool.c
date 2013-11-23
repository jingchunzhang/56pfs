/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_api.h"
#include "list.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
static list_head_t task;
#define THREADS 16
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	char path[256];
	list_head_t list;
} t_tool_task;

typedef struct {
	char *file;
	char *ip;
	int port;
} t_arg_upload;

void do_loop_dir(char *indir, char *ip, int port)
{
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(indir)) == NULL) 
	{
		fprintf(stderr, "opendir [%s] err %m\n", indir);
		return ;
	}
	while((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_name[0] == '.')
			continue;
		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s/%s", indir, dirp->d_name);
		struct stat filestat;
		if(stat(file, &filestat) < 0) 
		{
			fprintf(stderr, "stat error,filename:%s\n", file);
			continue;
		}
		if (S_ISDIR(filestat.st_mode))
			do_loop_dir(file, ip, port);
		else if (S_ISREG(filestat.st_mode))
		{
			if (filestat.st_size == 0)
				continue;
			t_api_info api;
			memset(&api, 0, sizeof(api));
			api.type = TASK_ADDFILE;
			api.addtype = ADD_PATH_BY_UPLOAD;
			snprintf(api.localfile, sizeof(api.localfile), "%s", file);
			int ret = operate_pfs(ip, port, &api);
			if (ret != PFS_OK)
				fprintf(stderr, "upload ERR %s:%d:%s\n", file, ret, api.errmsg);
		}
		else
			fprintf(stderr, "no process [%s]\n", file);
	}
	closedir(dp);
}

static int get_tool_task (t_tool_task ** tool)
{
	if (pthread_mutex_lock(&mutex))
	{
		fprintf(stderr, "pthread_mutex_lock err %m\n");
		return -1;
	}

	int get = -1;
	list_head_t *l;
	t_tool_task *tool0;
	list_for_each_entry_safe_l(tool0, l, &task, list)
	{
		list_del_init(&(tool0->list));
		get = 0;
		*tool = tool0;
		break;
	}

	if (pthread_mutex_unlock(&mutex))
		fprintf(stderr, "pthread_mutex_unlock err %m\n");

	return get;
}

void upload_thread(void *arg)
{
	t_arg_upload *u = (t_arg_upload *) arg;
	init_ns_cache();

	t_tool_task *tool = NULL;
	while (get_tool_task(&tool) == 0 && tool)
	{
		do_loop_dir(tool->path, u->ip, u->port);
		free(tool);
		tool = NULL;
	}
}

static int init_up_dir(char *indir)
{
	INIT_LIST_HEAD(&task);
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(indir)) == NULL) 
	{
		fprintf(stderr, "opendir [%s] err %m\n", indir);
		return -1;
	}
	while((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_name[0] == '.')
			continue;
		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s/%s", indir, dirp->d_name);
		struct stat filestat;
		if(stat(file, &filestat) < 0) 
		{
			fprintf(stderr, "stat error,filename:%s\n", file);
			continue;
		}
		if (S_ISDIR(filestat.st_mode))
		{
			t_tool_task * tool = malloc (sizeof(t_tool_task));
			if (tool == NULL)
			{
				fprintf(stderr, "malloc err %m\n");
				closedir(dp);
				return -1;
			}
			memset(tool, 0, sizeof(t_tool_task));
			strcpy(tool->path, file);
			INIT_LIST_HEAD(&(tool->list));
			list_add(&(tool->list), &task);
		}
	}
	closedir(dp);
	return 0;
}

int main(int c, char **v)
{
	if (c != 4)
	{
		fprintf(stderr, "Usage %s nameip nameport dir!\n", v[0]);
		return -1;
	}
	t_arg_upload u;
	memset(&u, 0, sizeof(u));
	u.file = v[3];
	u.ip = v[1];
	u.port = atoi(v[2]);

	if (init_up_dir(v[3]))
		return -1;

	int i = 0;
	pthread_t tid[THREADS] = {0x0};
	while (i < THREADS)
	{
		if(pthread_create(&tid[i], NULL, (void*(*)(void*))upload_thread, (void *)&u) != 0)
		{
			fprintf(stderr, "pthread_create err %m\n");
			return -1;
		}
		i++;
	}
	time_t s = time(NULL);
	fprintf(stdout, "start %s", ctime(&s));
	void *e;
	for (i = 0; i < THREADS; i++)
	{
		pthread_join(tid[i], &e);
	}

	time_t end = time(NULL);
	fprintf(stdout, "start %s end %s span = %ld", ctime(&s), ctime(&end), end - s);
	return 0;
}

