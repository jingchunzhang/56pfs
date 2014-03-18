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

int main(int c, char **v)
{
	if (c != 5)
	{
		fprintf(stderr, "Usage %s nameip nameport domain remotefile!\n", v[0]);
		return -1;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_DELFILE;
	snprintf(api.remotefile, sizeof(api.remotefile), "%s", v[4]);
	snprintf(api.group, sizeof(api.group), "%s", v[3]);
	int ret = operate_pfs(v[1], atoi(v[2]), &api);
	if (ret == PFS_OK)
		fprintf(stdout, "OK:%s:%s\n", v[4], api.remotefile);
	else
		fprintf(stderr, "del ERR %s:%s\n", v[4], api.errmsg);
	return 0;
}

