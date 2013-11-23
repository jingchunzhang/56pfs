/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

static char rootdir[1024][256];


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
		fprintf(stdout, "add %d %s\n", idx, s);
	}
	fclose(fpng);
	return 0;
}

int main(int c, char **v)
{
	init_rootdir();
	return 0;
}

