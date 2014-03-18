/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct
{
	uint32_t datasize; 
	uint32_t hashsize;
	uint32_t usedsize;
}t_hasht_shmhead;

typedef struct
{
	int count;
	int idx;
	char file[128];
}t_hasht_val;

typedef struct
{
	t_hasht_val v;
	uint32_t used;
	uint32_t next;
}t_hasht_data;

static t_hasht_shmhead *hasht_head = NULL;
static t_hasht_data * hasht_base = NULL;

uint32_t r5hash(const char *p) 
{
	uint32_t h = 0;
	while(*p) {
		h = h * 11 + (*p<<4) + (*p>>4);
		p++;
	}
	return h;
}

int init_hasht_hash(size_t size)
{
	size_t bigsize = size * sizeof(t_hasht_data) + sizeof(t_hasht_shmhead);
	hasht_head = (t_hasht_shmhead *)malloc(bigsize);
	if (hasht_head == NULL)
	{
		fprintf(stderr, "malloc err %ld %m\n", bigsize);
		return 0;
	}
	memset(hasht_head, 0, bigsize);

	char *base = (char *)hasht_head ;
	hasht_base =(t_hasht_data *)(base + sizeof(t_hasht_shmhead));
	hasht_head->hashsize = size /4;
	hasht_head->datasize = size - hasht_head->hashsize;
	hasht_head->usedsize = 0;

	return 0;
}

int find_hasht_node(char *text, t_hasht_data ** data0)
{
	unsigned int hash = r5hash(text)%hasht_head->hashsize;

	t_hasht_data * data = hasht_base + hash;
	while (1)
	{
		t_hasht_val *v = &(data->v);
		if (strcmp(text, v->file) == 0)
		{
			*data0 = data;
			return 0;
		}
		if (data->next == 0)
			break;
		data = hasht_base + data->next;
	}
	return -1;
}

int add_hasht_node(char *text, t_hasht_val *v)
{
	unsigned int hash = r5hash(text)%hasht_head->hashsize;

	t_hasht_data * data = hasht_base + hash;
	t_hasht_data * newone = NULL;
	if (data->used)
	{
		while (data->next)
		{
			if (data->used == 0)
			{
				newone = data;
				break;
			}
			data = hasht_base + data->next;
		}
		if (newone == NULL)
		{
			if (hasht_head->usedsize >= hasht_head->datasize)
				return -1;
			newone = hasht_base + hasht_head->hashsize + hasht_head->usedsize;
			data->next = hasht_head->hashsize + hasht_head->usedsize;
			hasht_head->usedsize++;
		}
	}
	else
		newone = data;
	memcpy(&(newone->v), v, sizeof(t_hasht_val));
	newone->used = 1;
	return 0;
}

static int sorthit(const void *p1, const void *p2)
{
	t_hasht_data * h1 = (t_hasht_data *) p1;
	t_hasht_data * h2 = (t_hasht_data *) p2;

	return h2->v.count > h1->v.count;
}

void sortdata()
{
	 qsort(hasht_base, hasht_head->hashsize + hasht_head->usedsize + 1, sizeof(t_hasht_data), sorthit);
}
