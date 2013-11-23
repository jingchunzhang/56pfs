/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "list.h"
#include "pfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "hasht.c"

#define LN __LINE__

static int total = 0;

static FILE *fpout = NULL;

static char rootdir[1024][256];

typedef struct {
	char file[256];
	list_head_t list;
} t_up_img;

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
		fprintf(stdout, "foepn %s err %m\n", ngconf);
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

static void do_upload(t_arg_upload *u, t_api_info *api)
{
	int ret = operate_pfs(u->ip, u->port, api);
	if (ret != PFS_OK)
		fprintf(fpout, "ERR:%s:%d:%s\n", api->localfile, ret, api->errmsg);
}

void do_loop_dir(t_arg_upload *u)
{
	FILE *fpin = fopen(u->file, "r");
	if (fpin == NULL)
	{
		fprintf(stdout, "fopen %s err %m\n", u->file);
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
			memset(buf, 0, sizeof(buf));
			continue;
		}
		*sep = 0x0;

		int idx = 0;

		if(buf[0] == 'p')
			idx = atoi(buf + 1);
		else if(strncmp(buf, "img.p", 5) == 0)
			idx = atoi(buf + 5);
		if (idx == 0)
		{
			memset(buf, 0, sizeof(buf));
			continue;
		}

		char *f = sep + 5;

		sep = strchr(f, '?');
		if (sep)
			*sep = 0x0;
		else
		{
			memset(buf, 0, sizeof(buf));
			continue;
		}

		char *e = f + strlen(f) - 4;
		if (strcmp(e, ".jpg"))
			continue;

		t_hasht_data *d;
		if (find_hasht_node(f, &d))
		{
			t_hasht_val v;
			memset(&v, 0, sizeof(v));
			v.idx = idx;
			v.count = 1;
			snprintf(v.file, sizeof(v.file), "%s", f);
			if (add_hasht_node(f, &v))
			{
				fprintf(stdout, "add_hasht_node err!\n");
				break;
			}
		}
		else
			d->v.count++;
		memset(buf, 0, sizeof(buf));
	}
	fclose(fpin);
	sortdata();
	t_hasht_data *base = hasht_base;
	int max = hasht_head->hashsize + hasht_head->usedsize + 1 - 60416;
	int i = 60416;
	time_t cur = time(NULL);
	time_t last = cur;
	for ( ; i < max; i++)
	{
		if (base->used == 0)
			continue;
		memset(api.localfile, 0, sizeof(api.localfile));
		snprintf(api.localfile, sizeof(api.localfile), "%s%s", rootdir[base->v.idx%1024], base->v.file);
		do_upload(u, &api);
		if ((i&0x3FF) == 0)
		{
			cur = time(NULL);
			fprintf(stdout, "%d %ld %ld\n", i, cur, last);
			last = cur;
		}
		base++;
		total++;
	}
}

void upload_thread(t_arg_upload *u)
{
	time_t sttime = time(NULL);
	init_ns_cache();
	do_loop_dir(u);
	time_t etime = time(NULL);

	fprintf(fpout, "S:%ld  E:%ld  span:%ld count:%d\n", sttime, etime, etime - sttime, total);
	fclose(fpout);
}

int main(int c, char **v)
{
	if (init_rootdir())
	{
		fprintf(stdout, "init_rootdir err %m\n");
		return -1;
	}
	if (c != 6)
	{
		fprintf(stdout, "Usage %s nameip nameport infile outfile group!\n", v[0]);
		return -1;
	}
	init_hasht_hash(8000000);
	t_arg_upload u;
	memset(&u, 0, sizeof(u));
	u.file = v[3];
	u.ip = v[1];
	u.port = atoi(v[2]);
	snprintf(u.group, sizeof(u.group), "%s", v[5]);

	fpout = fopen(v[4], "w");
	if (fpout == NULL)
	{
		fprintf(stdout, "%s open err %m!\n", v[4]);
		return -1;
	}
	upload_thread(&u);

	return 0;
}

