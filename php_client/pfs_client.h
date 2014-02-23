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

#ifndef PHP_PHPCLIENT_H
#define PHP_PHPCLIENT_H

extern zend_module_entry pfs_client_module_entry;
#define pfs_client_module_ptr &pfs_client_module_entry

#include <sys/types.h>

PHP_FUNCTION(pfs_remove_file);
PHP_FUNCTION(pfs_add_file);
PHP_FUNCTION(pfs_modify_file);
PHP_FUNCTION(pfs_query_other);

#endif /* PHP_PHPCLIENT_H */
