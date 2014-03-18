/*
* Copyright (C) 2012-2014 www.56.com email: jingchun.zhang AT renren-inc.com; jczhang AT 126.com ; danezhang77 AT gmail.com
* 
* 56VFS may be copied only under the terms of the GNU General Public License V3
*/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include <fcntl.h>
#include <sys/poll.h>
#include "atomic.h"
#include <stdint.h>
#include "list.h"
#include "log.h"
#include "myconfig.h"
#include "mybuff.h"

#define ID __FILE__
#define LN __LINE__
#define FUNC __FUNCTION__

extern struct conn *acon ;
extern int glogfd;


#define RECV_CLOSE 0x01   //do_recv need to close socket
#define RECV_ADD_EPOLLIN 0x02  //do_recv need to add fd EPOLLIN
#define RECV_ADD_EPOLLOUT 0x04 //do_recv need to add fd EPOLLOUT
#define RECV_ADD_EPOLLALL 0x06  //do_recv need to add fd EPOLLOUT and EPOLLIN
#define RECV_SEND 0x08  //do_recv need to send at once 

#define SEND_CLOSE 0x10 //do_send need to close socket
#define SEND_ADD_EPOLLIN 0x20 //do_send need to add fd EPOLLIN
#define SEND_ADD_EPOLLOUT 0x40 //do_send need to add fd EPOLLOUT
#define SEND_ADD_EPOLLALL 0x80 //do_send need to add fd EPOLLOUT and EPOLLIN

#define RET_OK 300
#define RET_SUCCESS 301
#define RET_CLOSE_HB 302  //4 detect hb
#define RET_CLOSE_MALLOC 303  //4 malloc err
#define RET_CLOSE_DUP 304  //dup connect

#define MAXSIG 20480
#define MAX_GROUP 64
#define MAX_DOMAIN 1024
#define MAX_IP_DOMAIN 64
#define MAX_NAMESERVER 64

extern int DIR1 ;
extern int DIR2 ;

#define MAXDISKS 32  //count of disk

enum ROLE {ROLE_UNKOWN = 0, ROLE_STORAGE, ROLE_NAMESERVER, ROLE_POSS_MASTER, SELF_IP, ROLE_CLIENT, ROLE_MAX};

enum MODE {CON_PASSIVE = 0, CON_ACTIVE};

enum SERVER_STAT {UNKOWN_STAT = 0, WAIT_SYNC, SYNCING, ON_LINE, OFF_LINE, STAT_MAX};

uint8_t self_stat;
uint8_t self_role;
uint32_t self_ip;
#endif
