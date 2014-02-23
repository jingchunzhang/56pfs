/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
void  sync_dir_thread(void * arg)
{
	t_pfs_tasklist *task = NULL;
	init_sync_list();
	init_sync_flag = 1;
	int ret = 0;
	while (1)
	{
		ret = pfs_get_task(&task, TASK_Q_SYNC_DIR_REQ);
		if (ret != GET_TASK_Q_OK)
		{
			sleep(5);
			continue;
		}
		t_task_base *base = (t_task_base*) &(task->task.base);
		do_sync_dir_req_sub(base);
		pfs_set_task(task, TASK_Q_SYNC_DIR_RSP);
	}
}
