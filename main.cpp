#include "sys/resource.h"
#include "main.h"

#include "ComLog.h"
#include "EPollServer.h"
#include <iostream>

using namespace std;

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

            cout << "1  Add <username> <password>  <group ID>  <acess level> " << endl;
            cout << "2  Modify <username> <active>  <iGroupID>  <Access Level > " << endl;
            cout << "3  Change password <username> <new password>  " << endl;	    
            cout << "4  Delete <username> " << endl;
            cout << "5  List DB " << endl;
	    cout << "0  Quit  " << endl;
	    cout << endl;

            char szUserName[SIZE_NAME];
            char szPassword[SIZE_NAME];
	    
	    int bActive, iGroupID, iAccessLevel;

	    int iRet;
	    
            int iSelection = 5;
            while (iSelection != 0) {
	      cout << endl;	
	      cout << "Enter Selection:" ;
		cin.clear();
                cin >> iSelection;
		cout << iSelection  << endl;
		
                switch (iSelection) {
                case 1:
		    cout << "Add User" << endl;		  
		    cout << "Enter UserName  Password  GroupID  iAccessLevel" << endl;
                    cin >> szUserName >> szPassword >> iGroupID >> iAccessLevel;
		    
                   iRet =  pCuserDB->AddUser(szUserName, szPassword, iGroupID, iAccessLevel);
		   if (iRet == USER_ALREADY_EXIST)
		     cout << "User Already Exist" << endl  << endl;
		   else
		      cout << "User Added "  << endl  << endl;
                    break;

                case 2:
		    cout << "Modify User" << endl;		  
		    cout << "Enter UserName  Active (0/1)  Group ID  Access Level " << endl;

		    cin >> szUserName >> bActive >> iGroupID >> iAccessLevel ;
                    pCuserDB->ModifyUser(szUserName, bActive, iGroupID, iAccessLevel);
                    break;
		    
                case 3:
		    cout << "Change User Passwordr" << endl;		  
		    cout << "Enter UserName  Password " << endl;
		  
                    cin >> szUserName >> szPassword ;
//		    cout << "Enter UserName  Password " << endl;
                    iRet = pCuserDB->ChangeUserPassword(szUserName, szPassword);
		    if(iRet == INVALID_USER_NAME)
		      cout << "Invalid Username" << endl<< endl;
		    else
		      cout << "Password changed "  << endl << endl;
                    break;
		    
            
                case 4:
		    cout << "Delete User" << endl;		  
		    cout << "Enter UserName" << endl;
		  
                    cin >> szUserName;
                    iRet = pCuserDB->DeleteUser(szUserName);
		    if (iRet == INVALID_USER_NAME)
			cout << "Invalid User name" <<  endl << endl;
		    else
			cout << "User Deleted " << endl << endl;		      
                    break;

                case 5:
		    cout << "List Users" << endl << endl;		  		  
                    pCuserDB->ListDB();
                    break;

                default:
		  continue;
                    break;

                } // switch
                if (iSelection == 0) {
		    pCuserDB->SaveUserFile();
                    delete pCuserDB;
		    
		    cout << " Successfull Termination";
		    exit(EXIT_SUCCESS);
                    break; // unreachable code
                } // if (iSelection == 0) 
            }//      while (iSelection != 0)
        }//if (!strncmp(argv[1], "U", 1)) {  // 'U' in upper case
        if (pCuserDB){
	   delete pCuserDB;
	   exit(EXIT_SUCCESS);
	}
    }  //    if (argc > 0)  // Look for "U" for user db mode

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
