
#include "sys/resource.h"

#include "main.h"

#include "ComLog.h"
#include "EPollServer.h"

////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
    // nfds is number of events (number of returned fd)

  EPOLL_CTOR_LIST SEpoll_Ctor;
  /*  initialize the structure here to construct the server
  SEpoll_Ctor.iLoadFactor = 
  SEpoll_Ctor.nReadThreads
  SEpoll_Ctor.nWriteThreads
  SEpoll_Ctor.nWriteThreads
  SEpoll_Ctor.iNumOFileDescriptors
  SEpoll_Ctor.szServerPort
  SEpoll_Ctor.iTimeOut
  SEpoll_Ctor.Local_addr
  SEpoll_Ctor.MaxByte
  SEpoll_Ctor.Open_Max
  */

 CComLog::instance().log("Starting EPOll Server", CComLog::Info);
 
 CEpollServer* pCEpoll = nullptr;
 
 pCEpoll = new CEpollServer(SEpoll_Ctor);

 if (!pCEpoll)
   exit(EXIT_FAILURE);
 
 if (pCEpoll->GetErrorCode() > 100)
 {
     CComLog::instance().log("Failure Getting an instance of EPOll Server", CComLog::Error);
     delete pCEpoll;
      exit(EXIT_FAILURE);
 }
 
 pCEpoll->PrepListener();
 if (pCEpoll->GetErrorCode() > 100)
 {
      CComLog::instance().log("Failure to Listen EPOll Server", CComLog::Error);
      delete pCEpoll;
      exit(EXIT_FAILURE);
 }
 
 pCEpoll->ProcessEpoll();
 if (pCEpoll->GetErrorCode() > 100)
 {
      CComLog::instance().log("Failure to Process EPOll Server", CComLog::Error);
      delete pCEpoll;
      exit(EXIT_FAILURE);
 }
 
 pCEpoll->TerminateThreads();
 
 delete pCEpoll;
 
 CComLog::instance().log("Success Termination of EPOll Server", CComLog::Info);
 
 exit(EXIT_SUCCESS);
}
////////////////////////////////////////////////////////////////////////////////////////////