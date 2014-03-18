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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int total = 0;

static FILE *fpout = NULL;
static FILE *fpok = NULL;

enum {GET_LAST_INODE, SET_LAST_INODE};

typedef struct {
	char *file;
	char *ip;
	int port;
	char group[128];
} t_arg_upload;

void set_get_last_inode(int type, long int ino)
{
	lseek(lastfd, 0, SEEK_SET);
	if (type == GET_LAST_INODE)
	{
		read(lastfd, &lastinode, sizeof(lastinode));
		if (lastinode == 0)
			valid = 1;
		fprintf(stderr, "last inode %ld\n", lastinode);
	}
	else
	{
		write(lastfd, &ino, sizeof(ino));
		fsync(lastfd);
	}
}

void open_last_node(char *file)
{
	lastfd = open(file, O_CREAT | O_RDWR , 0644);
	if (lastfd < 0)
	{
		fprintf(stderr, "open %s err %m\n", file);
		exit(0);
	}
}

void do_loop_dir(t_arg_upload *u)
{
	struct stat filestat;
	if (valid == 0)
	{
		if(stat(u->file, &filestat) < 0)
		{
			fprintf(stderr, "stat %s err %m\n", u->file);
			return;
		}
		if (lastinode != filestat.st_ino)
		{
			fprintf(stdout, "%s %ld %ld\n", u->file, lastinode, filestat.st_ino);
			return;
		}
		valid = 1;
	}
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(u->file)) == NULL) 
	{
		fprintf(stderr, "opendir [%s] err %m\n", u->file);
		return ;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_ADDFILE;
	api.addtype = ADD_PATH_BY_UPLOAD;
	api.nametype = ADD_SET_DOMAIN;
	snprintf(api.group, sizeof(api.group), "%s", u->group);
	while((dirp = readdir(dp)) != NULL) 
	{
		if (dirp->d_name[0] == '.')
			continue;
		char file[256] = {0x0};
		snprintf(file, sizeof(file), "%s/%s", u->file, dirp->d_name);
		if(stat(file, &filestat) < 0) 
		{
			fprintf(stderr, "stat error,filename:%s\n", file);
			continue;
		}
		if (S_ISDIR(filestat.st_mode))
		{
			t_arg_upload u0;
			memset(&u0, 0, sizeof(u0));
			memcpy(&u0, u, sizeof(u0));
			u0.file = file;
			do_loop_dir(&u0);
			if (valid)
				set_get_last_inode(SET_LAST_INODE, filestat.st_ino);
		}
		else if (S_ISREG(filestat.st_mode))
		{
			if (filestat.st_size == 0)
			{
				fprintf(fpout, "ZERO:%s\n", file);
				continue;
			}
			total++;
			memset(api.localfile, 0, sizeof(api.localfile));
			snprintf(api.localfile, sizeof(api.localfile), "%s", file);
			int ret = operate_pfs(u->ip, u->port, &api);
			if (ret != PFS_OK)
			{
				if (strncmp("remote", api.errmsg, 6))
					fprintf(fpout, "ERR:%s:%d:%s\n", file, ret, api.errmsg);
			}
		}
		else
			fprintf(stderr, "no process [%s]\n", file);
	}
	closedir(dp);
}

void upload_thread(t_arg_upload *u)
{
	set_get_last_inode(GET_LAST_INODE, 0);
	time_t sttime = time(NULL);
	init_ns_cache();
	do_loop_dir(u);
	time_t etime = time(NULL);

	fprintf(stdout, "S:%ld  E:%ld  span:%ld  count:%d\n", sttime, etime, etime - sttime, total);
}

int main(int c, char **v)
{
	if (c != 7)
	{
		fprintf(stderr, "Usage %s nameip nameport dir outfile group inodefile!\n", v[0]);
		return -1;
	}
	open_last_node(v[6]);
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

