/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include "c_api.h"
int	check_client_ip(int fd)
{
	return 0;
}

int active_send(pfs_cs_peer *peer, t_pfs_sig_head *h, t_pfs_sig_body *b)
{
	LOG(pfs_data_log, LOG_DEBUG, "send %d cmdid %x\n", peer->fd, h->cmdid);
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
	t_task_base *base = (t_task_base*) b;
	switch(h->cmdid)
	{
		case CMD_GET_FILE_RSP:
			if (h->bodylen != sizeof(t_task_base))
			{
				LOG(pfs_data_log, LOG_ERROR, "fd[%d] recv %s a bad bodylen[%d:%d]!\n", fd, str_cmd[h->cmdid], h->bodylen, sizeof(t_task_base));
				return RECV_CLOSE;
			}
			LOG(pfs_data_log, LOG_DEBUG, "fd[%d] recv %s %s\n", fd, str_cmd[h->cmdid], base->filename);
			return do_recv_task(fd, h, base); 

		case CMD_GET_FILE_REQ:  
			if (h->bodylen != sizeof(t_task_base))
			{
				LOG(pfs_data_log, LOG_ERROR, "fd[%d] recv %s a bad bodylen[%d:%d]!\n", fd, str_cmd[h->cmdid], h->bodylen, sizeof(t_task_base));
				return RECV_CLOSE;
			}
			LOG(pfs_data_log, LOG_DEBUG, "fd[%d] recv %s %s %s:%d\n", fd, str_cmd[h->cmdid], base->filename, ID, LN);
			do_send_task(fd, base, h);
			return RECV_ADD_EPOLLOUT;

		case CMD_PUT_FILE_REQ:
			if (check_client_ip(fd))
				return RECV_CLOSE;
			if (h->bodylen != sizeof(t_task_base))
			{
				LOG(pfs_data_log, LOG_ERROR, "fd[%d] recv %s a bad bodylen[%d:%d]!\n", fd, str_cmd[h->cmdid], h->bodylen, sizeof(t_task_base));
				return RECV_CLOSE;
			}
			return do_push_task(fd, h, base); 
			break;

		default:
			LOG(pfs_data_log, LOG_ERROR, "fd[%d] bad cmd close it [%x]\n", fd, h->cmdid);
			return RECV_CLOSE;
	}
	return RECV_ADD_EPOLLIN;
}
