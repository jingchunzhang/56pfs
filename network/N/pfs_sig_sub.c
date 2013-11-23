/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "c_api.h"
#include "pfs_localfile.h"

int get_valid_ip_4_up(t_pfs_domain *domains, char *sip)
{
	if (domains->index >= domains->ipcount)
		domains->index = 0;
	int j;
	int l = 0;
	pfs_fcs_peer *peer;
	for (j =0; j < domains->ipcount; j++)
	{
		if (find_ip_stat(domains->ips[j], &peer))
		{
			LOG(pfs_sig_log, LOG_ERROR, "ip [%s] not online\n", domains->sips[j]);
			continue;
		}
		l = snprintf(sip, 16, "%s", domains->sips[j]);
		sip[l] = 0x0;
		break;
	}
	if (j != domains->index)
	{
		if (find_ip_stat(domains->ips[domains->index], &peer))
			LOG(pfs_sig_log, LOG_ERROR, "ip [%s] not online\n", domains->sips[domains->index]);
		else
		{
			l = snprintf(sip, 16, "%s", domains->sips[domains->index]);
			sip[l] = 0x0;
		}
	}
	domains->index++;
	if (strlen(sip) == 0)
		return -1;
	LOG(pfs_sig_log, LOG_NORMAL, "domain [%s] ip [%s] be select\n", domains->domain, sip);
	return 0;
}

int get_dstip_group(char *buf, char *group, uint8_t type)
{
	t_pfs_domain *domains = NULL;
	if (strlen(group))
	{
		if (type == ADD_SET_GROUP)
			domains = get_domain_by_group(group);
		else
		{
			t_pfs_domain domains0;
			memset(&domains0, 0, sizeof(domains0));
			if (get_domain(group, &domains0) == 0)
				domains = &domains0;
		}
		if (domains == NULL)
		{
			LOG(pfs_sig_log, LOG_ERROR, "get_dstip_group '%s' err\n", group);
			return sprintf(buf, "ERROR group name %s", group);
		}
	}
	else
		domains = &(bestdomain[0]);
	char sip[16] = {0x0};
	if (get_valid_ip_4_up(domains, sip))
		return sprintf(buf, "N not found valid storage!");
	return sprintf(buf, "%s:%d", sip, g_config.up_port);
}

int get_dstip_group_modi_del(char *buf, char *domain)
{
	t_pfs_domain domains;
	memset(&domains, 0, sizeof(domains));
	if (strlen(domain))
	{
		if (get_domain(domain, &domains))
		{
			LOG(pfs_sig_log, LOG_ERROR, "get_dstip_group_modi_del err %s!\n", domain); 
			return sprintf(buf, "domain '%s' not found", domain);
		}
		char sip[16] = {0x0};
		if (get_valid_ip_4_up(&domains, sip))
			return sprintf(buf, "N not found valid storage!");
		return sprintf(buf, "%s:%d", sip, g_config.up_port);
	}
	LOG(pfs_sig_log, LOG_ERROR, "get_dstip_group_modi_del err domain null!\n"); 
	return sprintf(buf, "domain is NULL");
}

int active_send(pfs_fcs_peer *peer, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	LOG(pfs_sig_log, LOG_DEBUG, "send %d cmdid %x\n", peer->fd, h->cmdid);
	char obuf[2048] = {0x0};
	int n = 0;
	peer->hbtime = time(NULL);
	n = create_sig_msg(h->cmdid, h->status, b, obuf, h->bodylen);
	set_client_data(peer->fd, obuf, n);
	modify_fd_event(peer->fd, EPOLLOUT);
	return 0;
}

static int do_req(int fd, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	struct conn *curcon = &acon[fd];
	char obuf[2048] = {0x0};
	int n = 0;
	t_pfs_sig_body ob;
	memset(&ob, 0, sizeof(ob));
	int bodylen = 0;
	pfs_fcs_peer *peer = (pfs_fcs_peer *) curcon->user;
	peer->hbtime = time(NULL);

	switch(h->cmdid)
	{
		case HEARTBEAT_REQ:
			bodylen = sizeof(self_stat);
			memcpy(ob.body, &self_stat, sizeof(self_stat));
			n = create_sig_msg(HEARTBEAT_RSP, h->status, &ob, obuf, bodylen);
			set_client_data(fd, obuf, n);
			if (h->bodylen != 1)
				LOG(pfs_sig_log, LOG_ERROR, "fd[%d] recv a bad hb , no SERVER_STAT\n", fd);
			else
				peer->server_stat = *(b->body);
			LOG(pfs_sig_log, LOG_DEBUG, "fd[%d] recv a HB\n", fd);
			return RECV_SEND;

		case UPLOAD_REQ:
			LOG(pfs_sig_log, LOG_NORMAL, "fd [%d] UPLOAD_REQ!\n", fd);
			bodylen = get_dstip_group(ob.body, b->body, h->status);
			n = create_sig_msg(UPLOAD_RSP, h->status, &ob, obuf, bodylen);
			set_client_data(fd, obuf, n);
			modify_fd_event(fd, EPOLLOUT);
			peer->close = 1;
			return RECV_SEND;

		case MODI_DEL_REQ:
			LOG(pfs_sig_log, LOG_NORMAL, "fd [%d] MODI_DEL_REQ!\n", fd);
			bodylen = get_dstip_group_modi_del(ob.body, b->body);
			n = create_sig_msg(MODI_DEL_RSP, h->status, &ob, obuf, bodylen);
			set_client_data(fd, obuf, n);
			modify_fd_event(fd, EPOLLOUT);
			peer->close = 1;
			return RECV_SEND;

		case LOADINFO_REQ:
			LOG(pfs_sig_log, LOG_NORMAL, "fd [%d] LOADINFO_REQ!\n", fd);
			if (h->bodylen != sizeof(uint32_t) * 3)
				LOG(pfs_sig_log, LOG_ERROR, "fd[%d] recv a bad LOADINFO_REQ %d\n", fd, h->bodylen);
			else
			{
				memcpy(&(peer->taskcount), b->body, sizeof(uint32_t));
				memcpy(&(peer->load), b->body + sizeof(uint32_t), sizeof(uint32_t));
				memcpy(&(peer->diskfree), b->body + sizeof(uint32_t)*2, sizeof(uint32_t));
			}
			break;

		default:
			LOG(pfs_sig_log, LOG_ERROR, "fd[%d] recv a bad cmd [%x] status[%x]!\n", fd, h->cmdid, h->status);
		
	}
	return RECV_ADD_EPOLLIN;
}
