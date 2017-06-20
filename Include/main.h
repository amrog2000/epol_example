//*********************************************************************************************//
//https://github.com/linfan/TCP-IP-Network-basic-examples/blob/master/chapter9/EpollServer.c
//
//********************************************************************************************//

#pragma once

#include <string.h>         
#include <stdlib.h>         
#include <errno.h>
#include "sys/resource.h"

#include "EPollServer.h"

#include "ComLog.h"


// maximum received data byte
#define MAXBTYE     10      
#define OPEN_MAX    100
//#define LISTENQ     20  >> Changed to MaxEvents

#define SERV_PORT   10012
#define INFTIM      1000
#define LOCAL_ADDR  "127.0.0.1"

#define   SERVER_PORT_SIZE 10
// #define TIMEOUT     500

