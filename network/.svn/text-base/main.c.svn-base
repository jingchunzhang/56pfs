/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include "watchdog.h"
#include "myconfig.h"
#include "daemon.h"
#include "thread.h"
#include "common.h"
#include "fdinfo.h"
#include "version.h"
#include "log.h"
#include "pfs_so.h"
#include "pfs_init.h"
#include "pfs_task.h"
#include "pfs_agent.h"
#include "pfs_time_stamp.h"
#include "pfs_tmp_status.h"
#include "pfs_timer.h"

extern t_g_config g_config;
time_t pfs_start_time;  /*pfs Æô¶¯Ê±¼ä*/
int glogfd = -1;
char *iprole[] = {"unkown", "storage", "nameserver", "poss_master", "self_ip"}; 

static t_pfs_domain self_domain0;
t_pfs_domain *self_domain = &self_domain0;

static int init_glog()
{
	char *logname = myconfig_get_value("log_main_logname");
	if (!logname)
		logname = "./main_log.log";

	char *cloglevel = myconfig_get_value("log_main_loglevel");
	int loglevel = LOG_NORMAL;
	if (cloglevel)
		loglevel = getloglevel(cloglevel);
	int logsize = myconfig_get_intval("log_main_logsize", 100);
	int logintval = myconfig_get_intval("log_main_logtime", 3600);
	int lognum = myconfig_get_intval("log_main_lognum", 10);
	glogfd = registerlog(logname, loglevel, logsize, logintval, lognum);
	return glogfd;
}

static void gen_pidfile() {
	mode_t oldmask = umask(0);
	int fd = open("./watch-"_NS_".pid", O_CREAT | O_RDWR | O_TRUNC, 0644);
	if(fd > 0) {
		if(watchdog_pid != 0)	
			dprintf(fd, "%d\n", watchdog_pid);
		else 
			dprintf(fd, "%d\n", mainthread.pid);
		close(fd);
	}
	else {  
		printf("genpidfile fail, %m\n");
	}
	umask(oldmask);
}

static int parse_args(int argc, char* argv[]) {

	if(argc > 1) {
		if(!strncasecmp(argv[1], "-v", 2)) 
		{
			printf("Version:   %s\n", version_string);
			printf("Date   :   %s\n", compiling_date);	
			return -1;
		}
	}
	return 0;	
}
/* main thread loop */
static void main_loop(struct threadstat *thst) {
	while(!stop) {
		sleep(2);
		scan_delay_task();
		thread_reached(thst);
	}
}

/* server main */
#define ICALL(x)	if((err=x()) < 0) goto error
int main(int argc, char **argv) {
	if(parse_args(argc, argv))
		return 0;
	int err = 0;
	printf("Starting Server %s (%s)...%ld\n", version_string, compiling_date, nowtime());
	if(myconfig_init(argc, argv) < 0) {
		printf("myconfig_init fail %m\n");
		goto error;
	}
	daemon_start(argc, argv);
	umask(0);
	ICALL(start_watchdog);
	ICALL(init_log);
	ICALL(init_glog);
	ICALL(init_fdinfo);
	ICALL(delay_task_init);
	ICALL(init_thread);
	ICALL(init_global);
	ICALL(pfs_init);
	ICALL(init_task_info);
	self_role = get_self_role_ip_domain(&self_ip, self_domain);
	if (self_role <= ROLE_UNKOWN || self_role >= SELF_IP)
	{
		LOG(glogfd, LOG_ERROR, "get_self_role ERR!\n");
		fprintf(stderr, "get_self_role ERR!\n");
		goto error;
	}
	ICALL(init_load_tmp_status);
	if (self_role != ROLE_POSS_MASTER)
		ICALL(init_pfs_agent);
	char *srole = iprole[self_role];
	LOG(glogfd, LOG_NORMAL, "MY ROLE is %s\n", srole);

	t_thread_arg arg;
	memset(&arg, 0, sizeof(arg));
	arg.threadcount = 1;
	snprintf(arg.name, sizeof(arg.name), "./pfs_%s_sig.so", srole);
	LOG(glogfd, LOG_NORMAL, "prepare start %s\n", arg.name);
	arg.port = g_config.sig_port;
	arg.maxevent = myconfig_get_intval("pfs_sig_maxevent", 4096);
	if (init_pfs_thread(&arg))
		goto error;
	if (self_role == ROLE_STORAGE)
	{
		ICALL(init_storage_dir);
		t_thread_arg arg1;
		memset(&arg1, 0, sizeof(arg1));
		arg1.threadcount = 1;
		snprintf(arg1.name, sizeof(arg1.name), "./pfs_data.so");
		LOG(glogfd, LOG_NORMAL, "prepare start %s\n", arg1.name);
		arg1.port = g_config.data_port;
		arg1.flag = 1;
		arg1.maxevent = myconfig_get_intval("pfs_data_maxevent", 4096);
		if (init_pfs_thread(&arg1))
			goto error;

		t_thread_arg arg2;
		memset(&arg2, 0, sizeof(arg2));
		arg2.threadcount = 1;
		snprintf(arg2.name, sizeof(arg2.name), "./pfs_up.so");
		LOG(glogfd, LOG_NORMAL, "prepare start %s\n", arg2.name);
		arg2.port = g_config.up_port;
		arg2.flag = 1;
		arg2.maxevent = myconfig_get_intval("pfs_data_maxevent", 4096);
		if (init_pfs_thread(&arg2))
			goto error;
	}
	if (self_role == ROLE_STORAGE)
		ICALL(init_pfs_time_stamp);
	thread_jumbo_title();
	struct threadstat *thst = get_threadstat();
	if(start_threads() < 0)
		goto out;
	thread_reached(thst);
	gen_pidfile();	
	printf("Server Started\n");
	pfs_start_time = time(NULL);
	main_loop(thst);
out:
	printf("Stopping Server %s (%s)...\n", version_string, compiling_date);
	thread_reached(thst);
	stop_threads();
	myconfig_cleanup();
	fini_fdinfo();
	close_all_time_stamp();
	printf("Server Stopped.\n");
	return restart;
error:
	if(err == -ENOMEM) 
		printf("\n\033[31m\033[1mNO ENOUGH MEMORY\033[0m\n");
	
	printf("\033[31m\033[1mStart Fail.\033[0m\n");
	return -1;
}
