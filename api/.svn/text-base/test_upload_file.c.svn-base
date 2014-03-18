/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define THREADS 1
#define FILES 1

typedef struct {
	char *file;
	char *ip;
	int port;
} t_arg_upload;

void upload_thread(void * arg)
{
	init_ns_cache();
	t_arg_upload * u = (t_arg_upload *)arg;

	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_ADDFILE;
	api.addtype = ADD_PATH_BY_BNAME;
	api.nametype = ADD_SET_DOMAIN;
	snprintf(api.group, sizeof(api.group), "m2.pfs.56img.com");
	snprintf(api.localfile, sizeof(api.localfile), "%s", u->file);

	int i =0;
	while (i < FILES)
	{
		int ret = operate_pfs(u->ip, u->port, &api);
		if (ret != PFS_OK)
			fprintf(stderr, "upload ERR %s:%d:%s\n", u->file, ret, api.errmsg);
		else
			fprintf(stdout, "upload ok %s:%s\n", api.remotefile, api.group);
		i++;
	}
}

int main(int c, char **v)
{
	if (c != 4)
	{
		fprintf(stderr, "Usage %s nameip nameport localfile!\n", v[0]);
		return -1;
	}
	t_arg_upload u;
	memset(&u, 0, sizeof(u));
	u.file = v[3];
	u.ip = v[1];
	u.port = atoi(v[2]);

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
	void *e;
	for (i = 0; i < THREADS; i++)
	{
		pthread_join(tid[i], &e);
	}
	return 0;
}

