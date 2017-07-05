#include "EPollServer.h"

pthread_mutex_t *CEpollServer::pR_Mutex;
pthread_cond_t  *CEpollServer::pR_Condl;

pthread_mutex_t *CEpollServer::pW_Mutex;
pthread_cond_t  *CEpollServer::pW_Condl;

TASK_QUEUE CEpollServer::m_TaskQue = {0};
// struct task *CEpollServer::readhead = nullptr, *CEpollServer::readtail = nullptr;
// struct task *CEpollServer::writehead = nullptr, *CEpollServer::writetail = nullptr;

int 			CEpollServer::m_efd;
struct epoll_event 	CEpollServer::m_ev ;

THREAD_INFO*       CEpollServer::m_arrThreadInfo;
string 		   CEpollServer::m_strLogMsg;


//********************************************************************************************//
CEpollServer::CEpollServer(EPOLL_CTOR_LIST  CtorList):m_CtorList(CtorList)
{
//  m_CtorList = CtorList;

    pR_Thread = new pthread_t [m_CtorList.nReadThreads];  // array of Read threads
    pW_Thread = new pthread_t [m_CtorList.nWriteThreads]; // array of Write threads

    pR_Mutex = new pthread_mutex_t[m_CtorList.nReadThreads];   // array of Read  thread  mutexs
    pR_Condl = new pthread_cond_t[m_CtorList.nReadThreads];   // array of  Read conditional Variables

    pW_Mutex = new pthread_mutex_t[m_CtorList.nWriteThreads ]; 	// array of Read threads
    pW_Condl = new pthread_cond_t[m_CtorList.nWriteThreads ]; 	// array of  Write conditional Variables

    m_iNumOFileDescriptors = m_CtorList.iNumOFileDescriptors;
    m_MaxEvents = m_CtorList.MaxEvents;
    m_iTimeOut = m_CtorList.iTimeOut ;


    memset(m_szServerPort, '\0', SERVER_PORT_SIZE);
    strncpy(m_szServerPort, m_CtorList.szServerPort, SERVER_PORT_SIZE);;

    m_arrThreadInfo = new  THREAD_INFO[m_CtorList.nReadThreads + m_CtorList.nWriteThreads];

    int ii = 0;
    for (ii = 0; ii < m_CtorList.nReadThreads; ii++) {
        pthread_mutex_init(&pR_Mutex[i], nullptr);
        pthread_cond_init(&pR_Condl[i], nullptr);

        m_arrThreadInfo[ii].eState = TS_INACTIVE;

        pthread_create(&m_arrThreadInfo[ii].thread_id, nullptr, readtask, &ii);
        m_arrThreadInfo[ii].eState = TS_STARTING;
    }

    for (ii = m_CtorList.nReadThreads ; ii < (m_CtorList.nReadThreads + m_CtorList.nWriteThreads); ii++) {
        pthread_mutex_init(&pW_Mutex[i], nullptr);
        pthread_cond_init(&pW_Condl[i], nullptr);

        m_arrThreadInfo[ii].eState = TS_INACTIVE;

        pthread_create(&m_arrThreadInfo[ii].thread_id, nullptr, writetask, &ii);
        m_arrThreadInfo[ii].eState = TS_STARTING;
    }

    m_efd = epoll_create(m_MaxEvents);
    eventList = nullptr;
    
    eventList = new epoll_event[m_MaxEvents];
    if (!eventList) {
      m_iError = 1000;
      
     // ::TODO log error 
    }
    
    memset(m_szUserFileName, '\0', MAX_PATH);
    strcpy(m_szUserFileName, m_CtorList.szUserFileName);

    m_iReadMutexIndex 		= 0;
    m_iWriteMutexIndex  	= 0;
    
    m_TaskQue.readhead 		= nullptr;
    m_TaskQue.readtail 		= nullptr;
    
    m_TaskQue.uiReadTasksInQ  	= 0;
    m_TaskQue.uiWriteTasksInQ 	= 0;
    
    m_TaskQue.uiTotalWriteTasks = 0;
    m_TaskQue.uiTotalReadTasks  = 0;
    
    m_TaskQue.writehead 	= nullptr;
    m_TaskQue.readtail 		= nullptr;
    
    m_pCuserDB = nullptr;
    
    m_pCuserDB = new CuserDB(m_szUserFileName);
    if (!m_pCuserDB) {
     // ::TODO   log error 
    }
    
}
//********************************************************************************************//
CEpollServer::~CEpollServer()
{
// Terminate all threads

    CComLog::instance().log("Closing File descriptors", CComLog::Info);

    close(m_efd);  // Amro added this one...author did not close the file descriptor

    CComLog::instance().log("Deleting Events", CComLog::Info);
    if (eventList)
        delete[] eventList;

    CComLog::instance().log("Deleting Threads Array", CComLog::Info);

    if (pR_Thread)
        delete [] pR_Thread  ;
    if (pW_Thread)
        delete []  pW_Thread ;

    CComLog::instance().log("Deleting Mutex Array", CComLog::Info);
    if (pR_Mutex)
        delete []  pR_Mutex ;
    if (pW_Mutex)
        delete []  pW_Mutex ;

    CComLog::instance().log("Deleting Conditional Variables Array", CComLog::Info);
    if (pR_Condl)
        delete []  pR_Condl ;
    if (pW_Condl)
        delete []  pW_Condl ;

    CComLog::instance().log("Deleting Threads Array", CComLog::Info);
    if (m_arrThreadInfo)
        delete [] m_arrThreadInfo;

    if (m_pCuserDB){
     delete (m_pCuserDB); 
    }

    CComLog::instance().log("Destruction Completed", CComLog::Info);
}
//********************************************************************************************//
int CEpollServer::AuthenticateUser(char* szRecvBuffer)
{
   if (strlen(szRecvBuffer) < SIZE_OF_LOGIN_MESSAGE){
     return INVALID_LOGIN_MESSAGE;
   }
   
   char szUserName[SIZE_OF_USERNAME];
   char szPassword[SIZE_OF_PASSWORD];
/*
   char cMsgType;
   memmove(&cMsgType, szRecvBuffer, 1 );
   */
   if (*szRecvBuffer != 'L'){
     return INVALID_LOGIN_MESSAGE;
   }
   
   memset(szUserName, '\0', SIZE_OF_USERNAME);
   memset(szPassword,  '\0', SIZE_OF_PASSWORD);
   
   memmove(szUserName, szRecvBuffer+2 , 11 );
   memmove(szPassword, szRecvBuffer +14, 11);
   
   
   int iRet = m_pCuserDB->VerifyUser(szUserName, szPassword);
   
   return iRet;

  
}
//********************************************************************************************//
void CEpollServer::setnonblocking(int iSocket)
{
    int opts;
    if ((opts = fcntl(iSocket, F_GETFL)) < 0) {
        m_strLogMsg = "GETFL " + to_string( iSocket) + " Failed"; //"GETFL %d failed", iSocket);
        CComLog::instance().log(m_strLogMsg, CComLog::Error);
    }

    int iRet = 0;
    opts = opts | O_NONBLOCK;
    if (fcntl(iSocket, F_SETFL, opts) < 0) {
        m_strLogMsg = "SETFL " + to_string( iSocket) + " Failed"; //"GETFL %d failed", iSocket);
        CComLog::instance().log(m_strLogMsg, CComLog::Error);
        int iSendBuffSize = 9728;
        int iRecvBuffSize = 2048;


        iRet =   setsockopt(iSocket, SOL_SOCKET, SO_SNDBUF, &iSendBuffSize, sizeof iSendBuffSize);
	// check for errors by getiSocketopt
        iRet =   setsockopt(iSocket, SOL_SOCKET, SO_RCVBUF, &iRecvBuffSize, sizeof iRecvBuffSize);
        // check for errors by getiSocketopt
    }
}

//********************************************************************************************//

////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::PrepListener()
{

//  https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c

    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int s, sfd;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE| AI_V4MAPPED;     /* All interfaces */

    s = getaddrinfo (nullptr, m_szServerPort, &hints, &result);
    if (s != 0)
    {
//      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));

        m_iError = 510;  // enum all the errors
        return -1;

    }

    for (rp = result; rp != nullptr; rp = rp->ai_next)
    {
        m_Socket = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind (m_Socket, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {  // succefull bind
            break;
        }

        close (sfd);
    }

    if (rp == nullptr)
    {
        fprintf (stderr, "Could not bind\n");
        m_iError = 510;  // enum all the errors
        return -1;
    }

    freeaddrinfo (result);
    s = listen(m_Socket, m_MaxEvents);

    if (s == -1)
    {
        m_iError = 5210;  // enum all the errors
        return -1;
    }

    m_ev.data.fd = m_Socket;
    m_ev.events = EPOLLIN | EPOLLET;

    s = epoll_ctl (m_efd, EPOLL_CTL_ADD, sfd, &m_ev);

    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }


    return sfd;
}

////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::GetErrorCode()
{
    return m_iError;

}
////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::ProcessEpoll()
{

    struct sockaddr in_addr;
    socklen_t in_len = sizeof (in_addr);

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    int iRet = 0;

    int iRetry = 0;

    for(;;)
    {
        // waiting for epoll event
        m_nfds = epoll_wait(m_efd, eventList, m_MaxEvents, m_iTimeOut);
        // In case of edge trigger, must go over each event
        for (i = 0; i < m_nfds; ++i)
        {
            // Get new connection
            if (eventList[i].data.fd == m_Socket)
            {
                // accept the client connection
//                connfd = accept(m_Socket, (struct sockaddr*)&clientaddr, &in_len);
                connfd = accept(m_Socket, &in_addr, &in_len);

                if (connfd == -1) {
                    CComLog::instance().log("connfd < 0", CComLog::Error);
                    iRet++;
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { /* We have processed all incoming connections. */
                        break;
                    }
                    else {
                        CComLog::instance().log("accept error", CComLog::Error);
                        break;
                    }
                } // if (connfd == -1) {
                char szInBuffer[MAXBTYE ];
		int nRecv;
		string strMsg ;
                if (( nRecv = recv(m_Socket, szInBuffer, MAXBTYE, 0)) > 0) {		
		  nRecv = AuthenticateUser( szInBuffer);
		  if (nRecv != VALID_USER) {
		    strMsg.clear();
		    strMsg = "Connection Accept Error, following message was received ";
		    strMsg += szInBuffer;
                    CComLog::instance().log(strMsg, CComLog::Error);		    
		    strMsg.clear();
		    strMsg = "R";  // response message
		    strMsg += "R"; // rejection 
		    strMsg += to_string(nRecv);
		    send(m_Socket, strMsg.c_str(), 3, 0);
		     continue;
		  }
		}
		else{
		  continue;
		}
		strMsg = "R";  // response message
		strMsg += "A"; // accepted
		strMsg += to_string(nRecv);
		send(m_Socket, strMsg.c_str(), 3, 0);
		

                
                setnonblocking(connfd);
                m_strLogMsg = "[SERVER] connect from";
                m_strLogMsg += inet_ntoa(clientaddr.sin_addr);
                CComLog::instance().log(m_strLogMsg, CComLog::Debug);
//                CComLog::instance().log(m_strLogMsg);"[SERVER] connect from %s \n", inet_ntoa(clientaddr.sin_addr));

                iRet = getnameinfo (&in_addr, in_len,  hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
                if (iRet == 0) {
                    m_strLogMsg = "Accepted connection on descriptor:  " +  to_string(connfd) + "Host: " + hbuf + "Port: " + sbuf  ;
                    CComLog::instance().log(m_strLogMsg, CComLog::Info);

//                       CComLog::instance().log(m_strLogMsg);"Accepted connection on descriptor %d "
//                              "(host=%s, port=%s)\n", connfd, , );
                }

                m_ev.data.fd = connfd;
                // monitor in message, edge trigger
                m_ev.events = EPOLLIN | EPOLLET;
                // add fd to epoll queue
                epoll_ctl(m_efd, EPOLL_CTL_ADD, connfd, &m_ev);
            }  // if (eventList[i].data.fd == m_Socket)
            // Received data
            else if (eventList[i].events & EPOLLIN)
            {
                 if (eventList[i].data.fd < 0)
                     continue;
                m_strLogMsg = "[SERVER] put task: "+ to_string(eventList[i].data.fd) + " To read queue" ;

                CComLog::instance().log(m_strLogMsg, CComLog::Info);

//                new_task = (task*) malloc(sizeof(struct task));
                new_task = new task;
                new_task->data.fd = eventList[i].data.fd;
                new_task->next = nullptr;
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin before lock\n", pthread_self());
                // protect task queue (readhead/readtail)

		if (m_iReadMutexIndex++ > m_CtorList.nReadThreads)
                    m_iReadMutexIndex = 0;
		
                pthread_mutex_lock(&pR_Mutex[m_iReadMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin after lock\n", pthread_self());
                // the queue is empty
                if (m_TaskQue.readhead == nullptr)
                {
                    m_TaskQue.readhead = new_task;
                    m_TaskQue.readtail = new_task;
                }
                // queue is not empty
                else
                {
                    m_TaskQue.readtail->next = new_task;
                    m_TaskQue.readtail = new_task;
                }
        	m_TaskQue.uiReadTasksInQ++;
		m_TaskQue.uiTotalReadTasks++;

                pthread_cond_broadcast(&pR_Condl[ m_iReadMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin before unlock\n", pthread_self());
                pthread_mutex_unlock(&pR_Mutex[m_iReadMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin after unlock\n", pthread_self());
            }
            // Have data to send
            else if (eventList[i].events & EPOLLOUT)
            {
                if (eventList[i].data.ptr == nullptr)
                    continue;

                m_strLogMsg = "[SERVER] put task: " + to_string(((struct task*)eventList[i].data.ptr)->data.fd) + "To write queue";
                CComLog::instance().log(m_strLogMsg, CComLog::Info);


                new_task = new task;
                new_task->data.ptr = (struct user_data*)eventList[i].data.ptr;
                new_task->next = nullptr;
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout before lock\n", pthread_self());
//                pthread_mutex_lock(&pW_Mutex);

		if (m_iWriteMutexIndex++ > m_CtorList.nWriteThreads)
                    m_iWriteMutexIndex = 0;
		
                pthread_mutex_lock(&pW_Mutex[m_iWriteMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout after lock\n", pthread_self());
                // the queue is empty
                if (m_TaskQue.writehead == nullptr)
                {
                    m_TaskQue.writehead = new_task;
                    m_TaskQue.writetail = new_task;
                }
                // queue is not empty
                else
                {
                    m_TaskQue.writetail->next = new_task;
                    m_TaskQue.writetail = new_task;
                }
                
              	m_TaskQue.uiWriteTasksInQ++;
		m_TaskQue.uiTotalWriteTasks++;

                // trigger writetask thread
//                pthread_cond_broadcast(&pW_Condl);
                pthread_cond_broadcast(&pW_Condl[m_iWriteMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout before unlock\n", pthread_self());
                pthread_mutex_unlock(&pW_Mutex[m_iWriteMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout after unlock\n", pthread_self());
            }
            else
            {
                CComLog::instance().log("[SERVER] Error: unknown epoll event", CComLog::Error);
            }
        }
    }

    return 0;
}
//********************************************************************************************//
void *CEpollServer::readtask(void *args)
{
    int iThreadIndex = *((int*) &args);

    int fd = -1;
    int n, i;

    struct task* tmp = nullptr;

    m_arrThreadInfo[iThreadIndex].eState = TS_STARTED;

    struct user_data* data = nullptr;
    while(1)
    {
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask before lock\n", pthread_self());
        // protect task queue (readhead/readtail)
        pthread_mutex_lock(&pR_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask after lock\n", pthread_self());
        while(m_TaskQue.readhead == nullptr)
            // if condl false, will unlock mutex
            pthread_cond_wait(&pR_Condl[iThreadIndex ], &pR_Mutex[iThreadIndex ]);

        fd = m_TaskQue.readhead->data.fd;
        tmp = m_TaskQue.readhead;
        m_TaskQue.readhead = m_TaskQue.readhead->next;
        delete(tmp);
       	m_TaskQue.uiReadTasksInQ--;


        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask before unlock\n", pthread_self());
        pthread_mutex_unlock(&pR_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask after unlock\n", pthread_self());
        m_strLogMsg = "[SERVER] readtask %d handling " + to_string (pthread_self()) + to_string(fd);

        CComLog::instance().log(m_strLogMsg, CComLog::Info);

//        data = (user_data*)malloc(sizeof(struct user_data));
        data =  new (user_data);

        data->fd = fd;
        if ((n = recv(fd, data->line, MAXBTYE, 0)) < 0) {
            if (errno == ECONNRESET)
                close(fd);
            m_strLogMsg = "[SERVER] Error: readline failed: " ;
            m_strLogMsg +=  strerror(errno);
//            CComLog::instance().log(m_strLogMsg);"[SERVER] Error: readline failed: %s\n", strerror(errno));
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
            if (data != nullptr)
                delete(data);
        }
        else if (n == 0) {
            close(fd);
            m_strLogMsg = "[SERVER] Error: client" + to_string(fd) + "closed connection " ;
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
            if (data != nullptr)
                delete(data);
        }
        else {
            data->n_size = n;
            for (i = 0; i < n; ++i) {
                if (data->line[i] == '\n' || data->line[i] > 128)
                {
                    data->line[i] = '\0';
                    data->n_size = i + 1;
                }
            }

            m_strLogMsg = "[SERVER] readtask: "  + to_string (pthread_self()) + to_string(fd) + to_string(data->n_size) + data->line;
            CComLog::instance().log(m_strLogMsg, CComLog::Info);

            if (data->line[0] != '\0') {
                // modify monitored event to EPOLLOUT,  wait next loop to send respond
                m_ev.data.ptr = data;
                m_ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(m_efd, EPOLL_CTL_MOD, fd, &m_ev);
            }
        }
        if (m_arrThreadInfo[iThreadIndex].eState == TS_STOPPING ) {
            break;
        }
    }
    m_arrThreadInfo[iThreadIndex].eState == TS_TERMINATED;
}

//********************************************************************************************//
void *CEpollServer::writetask(void *args)
{

    int iThreadIndex = *((int*) &args);
    unsigned int n;
    // data to wirte back to client
    struct user_data *rdata = nullptr;
    while(1)
    {
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask before lock\n", pthread_self());
        pthread_mutex_lock(&pW_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask after lock\n", pthread_self());
        while(m_TaskQue.writehead == nullptr)
            // if condl false, will unlock mutex
            pthread_cond_wait(&pW_Condl[iThreadIndex ], &pW_Mutex[iThreadIndex ]);

        rdata = (struct user_data*)m_TaskQue.writehead->data.ptr;
        struct task* tmp = m_TaskQue.writehead;
        m_TaskQue.writehead = m_TaskQue.writehead->next;
        delete(tmp);
	m_TaskQue.uiWriteTasksInQ--;

        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask before unlock\n", pthread_self());
        pthread_mutex_unlock(&pW_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask after unlock\n", pthread_self());

        m_strLogMsg = "[SERVER] writetask " + to_string(pthread_self()) + "Sending: " +  to_string(rdata->fd )  + to_string(rdata->n_size) + rdata->line;
//      CComLog::instance().log(m_strLogMsg);"[SERVER] writetask %d sending %d : [%d] %s\n", pthread_self(), rdata->fd, rdata->n_size, rdata->line);
        CComLog::instance().log(m_strLogMsg, CComLog::Info);

        // send responce to client
        if ((n = send(rdata->fd, rdata->line, rdata->n_size, 0)) < 0)
        {
            if (errno == ECONNRESET)
                close(rdata->fd);
            m_strLogMsg = "[SERVER] Error: send responce failed: ";
            m_strLogMsg +=  strerror(errno);
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
        }
        else if (n == 0)
        {
            close(rdata->fd);
            CComLog::instance().log("[SERVER] Error: client closed connection.", CComLog::Info);
        }
        else
        {
            m_ev.data.fd = rdata->fd;
            m_ev.events = EPOLLIN | EPOLLET;
            epoll_ctl(m_efd, EPOLL_CTL_MOD, rdata->fd, &m_ev);
        }
        if (m_arrThreadInfo[iThreadIndex].eState == TS_STOPPING ) {
            break;
        }
    }
    delete(rdata);
    m_arrThreadInfo[iThreadIndex].eState = TS_TERMINATED;

    return nullptr;
}

//********************************************************************************************//
double CEpollServer::Get_CPU_Time(void)  // incomplete method....
{

    double user, sys;
    /*

        struct rusage myusage, childusage;
        if (getrusage(RUSAGE_SELF, &myusage) < 0)
        {
            CComLog::instance().log("[SERVER] Error: getrusage(RUSAGE_SELF) failed", CComLog::Error);
            return 0;
        }
        if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        {
            CComLog::instance().log("[SERVER] Error: getrusage(RUSAGE_CHILDREN) failed", CComLog::Error);
            return 0;
        }
        user = (double)myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec/1000000.0;
        user += (double)childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec/1000000.0;
        sys = (double)myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec/1000000.0;
        sys += (double)childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec/1000000.0;
        // show total user time and system time
        char  Msg[100];
        memset(Msg, '\0', 100);
        sprintf(Msg, "[SERVER] user time=%g, sys time=%g\n", user, sys);
        CComLog::instance().log(Msg, CComLog::Info);
        */
    return sys;
}
//********************************************************************************************//
int CEpollServer::TerminateThreads()
{
    std::string strExitMessage;

    int iJoined = 0;
    int iTotalThreads = m_CtorList.nReadThreads + m_CtorList.nWriteThreads;

    for (uint ii = 0;  ii < iTotalThreads; ii++ ) {  // send a Stop message to all threads
        m_arrThreadInfo[ii].eState = TS_STOPPING;
    }

    while (iJoined < iTotalThreads ) {
        // keep on checking for all terminated threads every three seconds

        for (uint ii = 0;  ii < iTotalThreads; ii++ ) {
            if ((m_arrThreadInfo[ii].eState == TS_JOINED))
                continue;
            if (m_arrThreadInfo[ii].eState == TS_TERMINATED) {

                pthread_join(m_arrThreadInfo[ii].thread_id, nullptr);
                m_arrThreadInfo[ii].eState = TS_JOINED;

                strExitMessage.clear();
                strExitMessage = "Thread Number: ";
                strExitMessage += to_string(m_arrThreadInfo[ii].thread_id);
                strExitMessage += " Joined";
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                strExitMessage = "Joined: ";
                strExitMessage += to_string(iJoined) + " Thread  out of: " + to_string(iTotalThreads);
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                iJoined++;
            }
        } // for loop
        sleep(3);
    } // while loop

    CComLog::instance().log("All Threads Joined", CComLog::Debug);

    int ii = 0;

    for (ii = 0; ii < m_CtorList.nReadThreads; ii++) {
        pthread_mutex_destroy(&pR_Mutex[i]);
        pthread_cond_destroy(&pR_Condl[i]);

    }
    CComLog::instance().log("All Read Mutexes Destroyed", CComLog::Debug);

    for (ii = m_CtorList.nReadThreads ; ii < (m_CtorList.nReadThreads + m_CtorList.nWriteThreads); ii++) {
        pthread_mutex_destroy(&pW_Mutex[i]);
        pthread_cond_destroy(&pW_Condl[i]);
    }
    CComLog::instance().log("All Write Mutexes Destroyed", CComLog::Debug);

    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TASK_QUEUE CEpollServer::GetQueueStatus()
{
  return m_TaskQue;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////