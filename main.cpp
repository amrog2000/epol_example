#include "sys/resource.h"
#include "main.h"

#include "ComLog.h"
#include "EPollServer.h"

#define	    SIZE_NAME  15

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

    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Starting EPOll Server", CComLog::Info);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);



    if (argc > 0) { // Look for 'U' for DB

      cout << "Running in user Database mode" << endl;;
      cout << "User Filename: " << argv[2];
    
	CuserDB* pCuserDB = nullptr;
        char szUserDBFile[MAX_PATH];

        memset(szUserDBFile, '\0', MAX_PATH);
        strcpy(szUserDBFile, argv[2]);
        pCuserDB = new CuserDB(szUserDBFile);

        if (!strncmp(argv[1], "U", 1)) {  // 'U' in upper case
            CComLog::instance().log("Starting Server for User Database Management", CComLog::Info);

            cout << "Enter A  <username> <password>  <group ID>  <acess level>  /* to add a record */ " << endl;
            cout << "Enter M  <username> <active /*1 or 0 */>  <iGroupID>  <Access Level >   /* to Modify user */ " << endl;
            cout << "Enter C  <username> <new password>    /* to Change password */ " << endl;	    
            cout << "Enter D  <username>   		   /* to Delete a record */ " << endl;
            cout << "Enter L              		   /* to List records int the DB */ " << endl;
	    cout << endl;

            char szUserName[SIZE_NAME];
            char szPassword[SIZE_NAME];
	    
	    int bActive, iGroupID, iAccessLevel;

	    int iRet;
	    
            int iSelection = 'x';
            while (iSelection != 'q') {
	      	cout << "Enter Selection:" << endl;
                cin >> iSelection ;
		cout << iSelection << endl;
		
                switch (iSelection) {
                case 'A':
                    cin >> szUserName >> szPassword >> iGroupID >> iAccessLevel;
                   iRet =  pCuserDB->AddUser(szUserName, szPassword, iGroupID, iAccessLevel);
		   if (iRet == USER_ALREADY_EXIST)
		     cout << "User Already Exist" << endl;
		   else
		      cout << "User Added ";
                    break;

                case 'C':
                    cin >> szUserName >> szPassword ;
                    iRet = pCuserDB->ChangeUserPassword(szUserName, szPassword);
		    if(iRet == INVALID_USER_NAME)
		      cout << "Invalid Username" << endl;
		    else
		      cout << "Password changed ";
                    break;
		    
                case 'M':
                    cin >> szUserName >> bActive >> iGroupID >> iAccessLevel ;
                    pCuserDB->ModifyUser(szUserName, bActive, iGroupID, iAccessLevel);
                    break;
            
                case 'D':
                    cin >> szUserName >> szPassword ;
                    iRet = pCuserDB->DeleteUser(szUserName);
		    if (iRet == INVALID_USER_NAME)
			cout << "Invalid User name" <<endl;
		    else
			cout << "User Deleted " <<endl;		      
                    break;

                case 'L':
                    pCuserDB->ListDB();
                    break;

                default:
                    break;

                } // switch
                if (iSelection == 'q') {
		    pCuserDB->SaveUserFile();
                    delete pCuserDB;
		    
		    cout << " Successfull Termination";
		    exit(EXIT_SUCCESS);
                    break;
                } // if (iSelection == 'q') 
            }//      while (iSelection != 'q')
        }//if (!strncmp(argv[1], "U", 1)) {  // 'U' in upper case
    }  //    if (argc > 0)  // Look for 'C' for Client  or else for Server

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

    pCEpoll->ProcessEpoll();   // main driver loop here
    if (pCEpoll->GetErrorCode() > 100)
    {
        CComLog::instance().log("Failure to Process EPOll Server", CComLog::Error);
        delete pCEpoll;
        exit(EXIT_FAILURE);
    }

    TASK_QUEUE TQueue =   pCEpoll->GetQueueStatus();
// output TQueue

    pCEpoll->TerminateThreads();

    delete pCEpoll;

    CComLog::instance().log("Success Termination of EPOll Server", CComLog::Info);

    exit(EXIT_SUCCESS);
}
////////////////////////////////////////////////////////////////////////////////////////////
