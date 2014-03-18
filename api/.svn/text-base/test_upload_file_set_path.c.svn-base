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

typedef struct {
	char *file;
	char *ip;
	int port;
} t_arg_upload;

void upload_thread(t_arg_upload *u)
{
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_ADDFILE;
	api.addtype = ADD_PATH_BY_UPLOAD;
	snprintf(api.localfile, sizeof(api.localfile), "%s", u->file);

	int ret = operate_pfs(u->ip, u->port, &api);
	if (ret == PFS_OK)
		fprintf(stdout, "OK:%s:%s\n", u->file, api.remotefile);
	else
		fprintf(stderr, "upload ERR %s:%d:%s\n", u->file, ret, api.errmsg);
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

	upload_thread(&u);

	return 0;
}

