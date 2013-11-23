/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "pfs_init.h"
#include "pfs_localfile.h"
#include <libgen.h>

extern list_head_t grouplist;
static char group_array[MAX_GROUP][128];
static t_pfs_domain bestdomain[MAX_GROUP];

static int find_ip_stat(uint32_t ip, pfs_fcs_peer **dpeer)
{
	list_head_t *hashlist = &(online_list[ALLMASK&ip]);
	pfs_fcs_peer *peer = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(peer, l, hashlist, hlist)
	{
		if (peer->ip == ip)
		{
			*dpeer = peer;
			return 0;
		}
	}
	return -1;
}

static int check_more_half_online(t_pfs_domain *domains)
{
	pfs_fcs_peer *peer;
	int j = 0;
	int online = 0;
	for (j =0; j < domains->ipcount; j++)
	{
		if (find_ip_stat(domains->ips[j], &peer))
		{
			LOG(pfs_sig_log, LOG_ERROR, "ip [%s] not online\n", domains->sips[j]);
			continue;
		}
		domains->diskfree = pfs_max(domains->diskfree, peer->diskfree);
		domains->load = pfs_min(domains->load, peer->load);
		domains->taskcount = pfs_min(domains->taskcount, peer->taskcount);
		online++;
	}
	if (online * 2 >= domains->ipcount)
	{
		LOG(pfs_sig_log, LOG_NORMAL, "domain [%s] %u:%u:%u\n", domains->domain, domains->diskfree, domains->load, domains->taskcount);
		return 0;
	}
	LOG(pfs_sig_log, LOG_ERROR, "domain [%s] too many ip not online %d:%d\n", domains->domain, online, domains->ipcount);
	return -1;
}

static int sortdomain(const void *p1, const void *p2)
{
	t_pfs_domain *s1 = (t_pfs_domain *)p1;
	t_pfs_domain *s2 = (t_pfs_domain *)p2;

	if (g_config.policy == POLICY_LOAD)
		return s1->load > s2->load;
	if (g_config.policy == POLICY_TASKCOUNT)
		return s1->taskcount > s2->taskcount;
	return s2->diskfree > s1->diskfree;
}

static int update_group_domain (t_pfs_group_list * group)
{
	t_pfs_domain *domain = NULL;
	t_pfs_domain *ddomain = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(domain, l, &(group->group.g_dlist), d_glist)
	{
		if (check_more_half_online(domain))
			continue;
		if (ddomain == NULL)
		{
			ddomain = domain;
			continue;
		}
		if (sortdomain(ddomain, domain))
			ddomain = domain;
	}
	if (ddomain)
	{
		int i = 0;
		int add_flag = 0;
		while (i < MAX_GROUP)
		{
			if (strlen(group_array[i]) == 0)
			{
				add_flag = 1;
				break;
			}
			if (strcmp(group->group.group, group_array[i]) == 0)
			{
				memcpy(&(bestdomain[i]), ddomain, sizeof(t_pfs_domain));
				break;
			}
			i++;
		}
		if (add_flag)
		{
			snprintf(group_array[i], 128, "%s", group->group.group);
			memcpy(&(bestdomain[i]), ddomain, sizeof(t_pfs_domain));
		}
		LOG(pfs_sig_log, LOG_NORMAL, "[%s:%s] %u:%u:%u\n", group->group.group, ddomain->domain, ddomain->diskfree, ddomain->load, ddomain->taskcount);
		return 0;
	}
	return -1;
}

static void update_sort_policy(time_t now)
{
	static time_t last = 0;
	if (now - last < 600)
		return;
	if (get_cfg_lock())
	{
		LOG(pfs_sig_log, LOG_ERROR, "get_cfg_lock err ,exit!\n");
		stop = 1;
		return;
	}
	int err = 0;
	last = now;
	t_pfs_group_list *group = NULL;
	list_head_t *l;
	list_for_each_entry_safe_l(group, l, &grouplist, g_glist)
	{
		if (update_group_domain(group))
			err = 1;
	}
	if (err)
		last = 0;
	else
		LOG(pfs_sig_log, LOG_NORMAL, "update_sort_policy ok!\n");
	release_cfg_lock();
}

static t_pfs_domain * get_domain_by_group(char *group)
{
	int i = 0;
	while (i < MAX_GROUP)
	{
		if (strlen(group_array[i]) == 0)
			return NULL;
		if (strcmp(group, group_array[i]) == 0)
			return &(bestdomain[i]);
		i++;
	}
	return NULL;
}

