/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2013 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Christian Cartus <cartus@atrior.de>                          |
   +----------------------------------------------------------------------+
 */
 
/* $Id$ */

/* This has been built and tested on Linux 2.2.14 
 *
 * This has been built and tested on Solaris 2.6.
 * It may not compile or execute correctly on other systems.
 */

#include "pfs_api.h"
#include "config.h"

#include "php.h"

#include <errno.h>

#include "pfs_client.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_str.h"
#include "php_ini.h"

#define ADD_DOMAIN_BNAME 3
#define DEFAULT_UP_PORT 49711
#ifndef INADDR_NONE 
#define INADDR_NONE ((unsigned long) -1) 
#endif
static PHP_MINFO_FUNCTION(pfs_client)
{
}

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_pfs_remove_file, 0, 0, 3)
	ZEND_ARG_INFO(0, serverport)
	ZEND_ARG_INFO(0, domain)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pfs_add_file, 0, 0, 4)
	ZEND_ARG_INFO(0, serverport)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, file)
	ZEND_ARG_INFO(0, flag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pfs_query_other, 0, 0, 3)
	ZEND_ARG_INFO(0, serverport)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, flag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pfs_modify_file, 0, 0, 4)
	ZEND_ARG_INFO(0, server)
	ZEND_ARG_INFO(0, domain)
	ZEND_ARG_INFO(0, localfile)
	ZEND_ARG_INFO(0, remotefile)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pfs_client_functions[]
 */
static const zend_function_entry pfs_client_functions[] = {
	PHP_FE(pfs_remove_file,	arginfo_pfs_remove_file)
	PHP_FE(pfs_add_file,	arginfo_pfs_add_file)
	PHP_FE(pfs_query_other,	arginfo_pfs_query_other)
	PHP_FE(pfs_modify_file,	arginfo_pfs_modify_file)
	{NULL, NULL, NULL}  /* Must be the last line */
};
/* }}} */

/* {{{ pfs_client_module_entry
 */
zend_module_entry pfs_client_module_entry = {
	STANDARD_MODULE_HEADER,
	"pfs_client",
	pfs_client_functions, 
	NULL,
	NULL,
	NULL,
	NULL,
	PHP_MINFO(pfs_client),
	"1.0",
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(pfs_client)

static int get_ip_port(char *s, char *d, int *port)
{
	char *t = strchr(s, ':');
	if (t)
	{
		*t = 0x0;
		*port = atoi(t+1);
	}
	if (get_uint32_ip(s, d) == INADDR_NONE)
		return -1;
	return 0;
}

PHP_FUNCTION(pfs_remove_file)
{
	char *server;
	long server_len;

	char *group;
	long group_len;

	char *file;
	long file_len;

	char *errmsg[] = {"OK", "zend_parse_parameters ERR", "domain err:unknown host"};
	
	array_init(return_value);
	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &server, &server_len, &group, &group_len, &file, &file_len))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 1);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[1], strlen(errmsg[1]), 1);
		return;
	}
	int port = DEFAULT_UP_PORT;
	char ip[16] = {0x0};
	if (get_ip_port(server, ip, &port))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 2);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[2], strlen(errmsg[2]), 1);
		return;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_DELFILE;
	snprintf(api.remotefile, sizeof(api.remotefile), "%s", file);
	snprintf(api.group, sizeof(api.group), "%s", group);
	int ret = operate_pfs(ip, port, &api);
	add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
	add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), pfserrmsg[ret], strlen(pfserrmsg[ret]), 1);
	return;
}

PHP_FUNCTION(pfs_add_file)
{
	char *server;
	long server_len;

	char *file;
	long file_len;

	char *group;
	long group_len;

	long flag = 0;

	array_init(return_value);
	char *errmsg[] = {"OK", "zend_parse_parameters ERR", "domain err:unknown host"};
	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sssl", &server, &server_len, &group, &group_len, &file, &file_len, &flag))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 1);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[1], strlen(errmsg[1]), 1);
		return;
	}
	int port = DEFAULT_UP_PORT;
	char ip[16] = {0x0};
	if (get_ip_port(server, ip, &port))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 2);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[2], strlen(errmsg[2]), 1);
		return;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_ADDFILE;
	if (flag == ADD_PATH_BY_BNAME)
		api.addtype = ADD_PATH_BY_BNAME;
	else if (flag == ADD_SET_DOMAIN)
		api.nametype = ADD_SET_DOMAIN;
	else if (flag == ADD_DOMAIN_BNAME)
	{
		api.addtype = ADD_PATH_BY_BNAME;
		api.nametype = ADD_SET_DOMAIN;
	}

	snprintf(api.localfile, sizeof(api.localfile), "%s", file);
	snprintf(api.group, sizeof(api.group), "%s", group);
	int ret = operate_pfs(ip, port, &api);
	if (ret == PFS_OK)
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "domain", sizeof("domain"), api.group, strlen(api.group), 1);
		add_assoc_stringl_ex(return_value, "remotefile", sizeof("remotefile"), api.remotefile, strlen(api.remotefile), 1);
	}
	else
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "errmsg", sizeof("errmsg"), api.errmsg, strlen(api.errmsg), 1);
		if (ret >= 0 && ret <= PFS_STORAGE_FILE)
			add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), pfserrmsg[ret], strlen(pfserrmsg[ret]), 1);
	}
}

PHP_FUNCTION(pfs_query_other)
{
	char *server;
	long server_len;

	char *group;
	long group_len;

	long flag = 0;

	array_init(return_value);
	char *errmsg[] = {"OK", "zend_parse_parameters ERR", "domain err:unknown host", "flag range err"};
	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &server, &server_len, &group, &group_len, &flag))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 1);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[1], strlen(errmsg[1]), 1);
		return;
	}
	int port = DEFAULT_UP_PORT;
	char ip[16] = {0x0};
	if (get_ip_port(server, ip, &port))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 2);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[2], strlen(errmsg[2]), 1);
		return;
	}

	if (flag < ADD_QUERY_DIR)
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 3);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[3], strlen(errmsg[3]), 1);
		return;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_DELFILE;
	api.addtype = flag;

	snprintf(api.remotefile, sizeof(api.remotefile), "blank");
	snprintf(api.group, sizeof(api.group), "%s", group);
	int ret = operate_pfs(ip, port, &api);
	if (ret == PFS_OK)
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "domain", sizeof("domain"), api.group, strlen(api.group), 1);
		add_assoc_stringl_ex(return_value, "remotedir", sizeof("remotedir"), api.remotefile, strlen(api.remotefile), 1);
	}
	else
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "errmsg", sizeof("errmsg"), api.errmsg, strlen(api.errmsg), 1);
	}
}

PHP_FUNCTION(pfs_modify_file)
{
	char *server;
	long server_len;

	char *file;
	long file_len;

	char *group;
	long group_len;

	char *rfile;
	long rfile_len;

	array_init(return_value);
	char *errmsg[] = {"OK", "zend_parse_parameters ERR", "domain err:unknown host"};
	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssss", &server, &server_len, &group, &group_len, &file, &file_len, &rfile, &rfile_len))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 1);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[1], strlen(errmsg[1]), 1);
		return;
	}
	int port = DEFAULT_UP_PORT;
	char ip[16] = {0x0};
	if (get_ip_port(server, ip, &port))
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), 2);
		add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), errmsg[2], strlen(errmsg[2]), 1);
		return;
	}
	t_api_info api;
	memset(&api, 0, sizeof(api));
	api.type = TASK_MODYFILE;
	snprintf(api.localfile, sizeof(api.localfile), "%s", file);
	snprintf(api.remotefile, sizeof(api.remotefile), "%s", rfile);
	snprintf(api.group, sizeof(api.group), "%s", group);
	int ret = operate_pfs(ip, port, &api);
	if (ret == PFS_OK)
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "domain", sizeof("domain"), api.group, strlen(api.group), 1);
		add_assoc_stringl_ex(return_value, "remotefile", sizeof("remotefile"), api.remotefile, strlen(api.remotefile), 1);
	}
	else
	{
		add_assoc_long_ex(return_value, "retcode", sizeof("retcode"), ret);
		add_assoc_stringl_ex(return_value, "errmsg", sizeof("errmsg"), api.errmsg, strlen(api.errmsg), 1);
		if (ret >= 0 && ret <= PFS_STORAGE_FILE)
			add_assoc_stringl_ex(return_value, "retmsg", sizeof("retmsg"), pfserrmsg[ret], strlen(pfserrmsg[ret]), 1);
	}
}

