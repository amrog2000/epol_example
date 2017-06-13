//*********************************************************************************************//
//https://github.com/linfan/TCP-IP-Network-basic-examples/blob/master/chapter9/EpollServer.c
//
//********************************************************************************************//
// socket interface
#include <sys/socket.h>     
// epoll interface
#include <sys/epoll.h>      
// struct sockaddr_in
#include <netinet/in.h>     
// IP addr convertion
#include <arpa/inet.h>      
// File descriptor controller
#include <fcntl.h>          
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
// bzero()
#include <string.h>         
// malloc(), free()
#include <stdlib.h>         
#include <errno.h>
//extern int errno;

// maximum received data byte
#define MAXBTYE     10      
#define OPEN_MAX    100
//#define LISTENQ     20  >> Changed to MaxEvents

#define SERV_PORT   10012
#define INFTIM      1000
#define LOCAL_ADDR  "127.0.0.1"
// #define TIMEOUT     500

enum tstate {
    TS_INACTIVE,
    TS_STARTING,
    TS_STARTED,
    TS_ALIVE,
    TS_TERMINATED,
    TS_JOINED
};

typedef struct thread_info
{   /* Used as argument to thread_start() */
    pthread_t 	thread_id;        /* ID returned by pthread_create() */
    enum 	tstate eState;
    int       	iThread_num;       /* Application-defined thread # */
    char     	*thread_message;      /* Saying Hello */
} THREAD_INFO;

// task item in thread pool
struct task                          
{
    // file descriptor or user_data
    epoll_data_t data;        
    // next task
    struct task* next;               
};

// for data transporting
struct user_data {
    int fd;
    // real received data size
    unsigned int n_size;             
    // content received
    char line[MAXBTYE];              
};

typedef struct CEpollCtorList {
  
  int MaxByte;     	// 10
  int Open_Max;    	//100
  int MaxEvents;     	//20
  int Serv_Port;   	//10012
  int _INFTIM;      	//1000
  int Local_addr; 	// "127.0.0.1"
  int iTimeOut;     	//500  
  
  int iNumOFileDescriptors;
  
  int 	iMaxReadThreads;  // Thread Pool for Read
  int 	iMaxWriteThreads; // Thread Pool for Write
  
  int   iServerPort;

  int   iLoadFactor;  // Max load to a given thread until a new thread is added from the pool
  
}EPOLL_CTOR_LIST;

class CEpoll {
  
public:
 CEpoll(EPOLL_CTOR_LIST);
 ~CEpoll();
  
private:  // yes yes it is by default
  
    int i, maxi, m_nfds;               
//    int listenfd;
    int  m_Socket;
    int  connfd;
    
static    int m_efd;
    int m_MaxEvents;
    
    int   m_iTimeOut;
    // task node
    struct task *new_task = NULL;    
    socklen_t clilen;
    
    int m_iServerPort;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

static    void *readtask(void *args);
static    void *writetask(void *args);
    // epoll descriptor from epoll_create()
    int m_epfd;                            
    // register epoll_ctl()
static    struct epoll_event ev;               
    // store queued events from epoll_wait()
//    struct epoll_event events[m_MaxEvents];
     struct epoll_event* events;
    
    int   m_iNumOFileDescriptors;
/*
    pthread_t tid1, tid2;            
    pthread_t tid3, tid4;            */


    pthread_t  *pRThread;  // == tid1 or tid2
    pthread_t  *pWThread;  // == tid1 or tid2
    
static    pthread_mutex_t *r_mutex;             
static    pthread_cond_t  *r_condl;

static    pthread_mutex_t *w_mutex;             
static    pthread_cond_t  *w_condl;

static    struct task *readhead, *readtail;
static    struct task *writehead, *writetail;
    
    EPOLL_CTOR_LIST m_CtorList;
    
    int m_iError;
    void setnonblocking(int sock);
    
    int m_iReadMutexIndex;
    int m_iWriteMutexIndex;

    
public:    
    int  PrepListener();
    
    int GetErrorCode();
    int ProcessEpoll();
    int TerminateThreads();
    
    THREAD_INFO* m_arrThreadInfo;
    
};

pthread_mutex_t *CEpoll::r_mutex;             
pthread_cond_t  *CEpoll::r_condl;

pthread_mutex_t *CEpoll::w_mutex;             
pthread_cond_t  *CEpoll::w_condl;

struct task *CEpoll::readhead = NULL, *CEpoll::readtail = NULL;
struct task *CEpoll::writehead = NULL, *CEpoll::writetail = NULL;

int CEpoll::m_efd;
struct epoll_event CEpoll::ev ;               

