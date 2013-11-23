/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef __NM_C_API_H_
#define __NM_C_API_H_
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
void SetInt(uint32_t key, uint32_t val);
void IncInt(uint32_t key, uint32_t val);
void SetStr(uint32_t key, char * val);
#ifdef __cplusplus
}
#endif
#endif
