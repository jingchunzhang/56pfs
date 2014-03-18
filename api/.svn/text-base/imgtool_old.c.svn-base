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
#include <sys/types.h>
#include <dirent.h>

static int total = 0;

static FILE *fpout = NULL;

static char rootdir[1024][256];

typedef struct {
	char *file;
	char *ip;
	int port;
	char group[128];
} t_arg_upload;

static char *ltrim(char *str)
{
	while( *str != 0x0 && *str == ' ') str++;
	return str;
}

static int init_rootdir()
{
	memset(rootdir, 0, sizeof(rootdir));
	int l = strlen("server_name p");
	char *ngconf = "/home/nginx/conf/nginx.conf";
	FILE *fpng = fopen(ngconf, "r");
	if (fpng == NULL)
	{
		fprintf(stderr, "foepn %s err %m\n", ngconf);
		return -1;
	}
	char buf[1024] = {0x0};
	while (fgets(buf, sizeof(buf), fpng))
	{
		char *d = ltrim(buf);
		if(*d == '#')
		{
			memset(buf, 0, sizeof(buf));
			continue;
		}
		char *s = strstr(d, "server_name p");
		if (s == NULL)
		{
			memset(buf, 0, sizeof(buf));
			continue;
		}
		char *e = s + l;
		int idx = atoi(e);

		memset(buf, 0, sizeof(buf));
		while (fgets(buf, sizeof(buf), fpng))
		{
			d = ltrim(buf);
			if(*d == '#')
			{
				memset(buf, 0, sizeof(buf));
				continue;
			}
			s = strstr(d, "root ");
			if (s == NULL)
			{
				memset(buf, 0, sizeof(buf));
				continue;
			}
			break;
		}
		s += 5;
		e = strchr(s, ';');
		if (e)
			*e = 0x0;
		snprintf(rootdir[idx%1024], 256, "%s", s);
	}
	fclose(fpng);
	return 0;
}

void do_loop_dir(t_arg_upload *u)
{
	FILE *fpin = fopen(u->file, "r");
	if (fpin == NULL)
	{
		fprintf(stderr, "fopen %s err %m\n", u->file);
		return;
	}

	char buf[256] = {0x0};
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_ADDFILE;
	api.addtype = ADD_PATH_BY_UPLOAD;
	api.nametype = ADD_SET_DOMAIN;
	snprintf(api.group, sizeof(api.group), "%s", u->group);
	while(fgets(buf, sizeof(buf), fpin)) 
	{
		char *sep = strchr(buf, ' ');
		if (sep == NULL)
		{
			fprintf(fpout, "ERR:%s\n", buf);
			memset(buf, 0, sizeof(buf));
			continue;
		}
		char *last = strchr(buf, '\n');
		if (last)
			*last = 0x0;
		*sep = 0x0;

		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s%s", rootdir[atoi(buf)&0x3FF], sep+1);
		struct stat filestat;
		if(stat(file, &filestat) < 0) 
		{
			fprintf(fpout, "stat error,filename:%s %m\n", file);
			memset(buf, 0, sizeof(buf));
			continue;
		}
		if (filestat.st_size == 0)
		{
			fprintf(fpout, "ZERO:%s\n", file);
			memset(buf, 0, sizeof(buf));
			continue;
		}
		total++;
		memset(api.localfile, 0, sizeof(api.localfile));
		snprintf(api.localfile, sizeof(api.localfile), "%s", file);
		int ret = operate_pfs(u->ip, u->port, &api);
		if (ret != PFS_OK)
		{
			ret = operate_pfs(u->ip, u->port, &api);
			if (ret != PFS_OK)
				fprintf(fpout, "ERR:%s:%d:%s\n", file, ret, api.errmsg);
		}
	}
	fclose(fpin);
	fclose(fpout);
}

void upload_thread(t_arg_upload *u)
{
	time_t sttime = time(NULL);
	init_ns_cache();
	do_loop_dir(u);
	time_t etime = time(NULL);

	fprintf(stdout, "S:%ld  E:%ld  span:%ld  count:%d\n", sttime, etime, etime - sttime, total);
}

int main(int c, char **v)
{
	if (init_rootdir())
	{
		fprintf(stderr, "init_rootdir err %m\n");
		return -1;
	}
	if (c != 6)
	{
		fprintf(stderr, "Usage %s nameip nameport infile outfile group!\n", v[0]);
		return -1;
	}
	t_arg_upload u;
	memset(&u, 0, sizeof(u));
	u.file = v[3];
	u.ip = v[1];
	u.port = atoi(v[2]);
	snprintf(u.group, sizeof(u.group), "%s", v[5]);

	fpout = fopen(v[4], "w");
	if (fpout == NULL)
	{
		fprintf(stderr, "%s open err %m!\n", v[4]);
		return -1;
	}
	upload_thread(&u);

	return 0;
}

