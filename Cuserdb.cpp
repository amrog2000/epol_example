
#include "Cuserdb.h"
#include "ComLog.h"

CuserDB::CuserDB(char* szUserFileName)
{

    struct stat st = {0};

    if (stat("../Users/", &st) == -1) {
        mkdir("../Users/", 0700);
    }


    string strUserDBFile;
    strUserDBFile.clear();

    strUserDBFile = "../Users/";
    strUserDBFile += "szUserFileName";

    m_ifd = open(strUserDBFile.c_str(), O_CREAT|O_RDWR|O_APPEND, S_IRWXU);

    USER_RECORD UserRecord = {0};

    m_iSizeOfUserRecord = sizeof (USER_RECORD);
    int iRecSize = 0;

    if (m_ifd == -1) {
        CComLog::instance().log("Open User DB File...Error Opening File: ", CComLog::Error);
        CComLog::instance().log(strUserDBFile, CComLog::Error);
        // Set error code and exit
    }
    else {// load users in Map
        int nCount = 0;
        while ((iRecSize = read(m_ifd,  &UserRecord, m_iSizeOfUserRecord)) > 0) {
            m_itUserMap = m_UserMap.find(UserRecord.szUserName);
            if (m_itUserMap == m_UserMap.end()) { // Not found
                m_UserMap.insert(pair<string, USER_RECORD> ( UserRecord.szUserName, UserRecord));
                nCount++;
            }
        } // while
        string strMsg;
        strMsg = "Inserted: " + to_string(nCount) + " User Records";
        CComLog::instance().log(strMsg, CComLog::Info);
    } // else
}
//********************************************************************************************//
CuserDB::~CuserDB()
{
  SaveUserFile();
  close(m_ifd);
  m_UserMap.clear();
}
//********************************************************************************************//
int CuserDB::VerifyUser(char* szUsername, char* szPassword)
{
   USER_RECORD UserRecord = {0};

   m_itUserMap = m_UserMap.find(szUsername);
   
   if (m_itUserMap == m_UserMap.end()) {
    return INVALID_USER_NAME; 
   }
   
   UserRecord = m_itUserMap->second;
   
   if (strcmp(szPassword, UserRecord.szPassword)) {
     return INVALID_PASSWORD;
   }
   
   if (UserRecord.bActive) {
     return USER_INACTIVE;
   }
   
   return VALID_USER;
  
}
//********************************************************************************************//
int CuserDB::AddUser(char* szUsername, char* szPassword, int iGroupID, int iAccessLevel)
{
  USER_RECORD UserRecord = {0};
  pair <MapUserDB::iterator, bool> pairInsert;
  
  UserRecord.bActive = true;
  UserRecord.iAccessLevel = iAccessLevel;
  strcpy(UserRecord.szUserName, szUsername);
  strcpy(UserRecord.szPassword, szPassword);
  UserRecord.iGroupID = iGroupID;
  
  strcpy(UserRecord.szDateAccountCreated, CComLog::instance().GetFormatedDateTime());
  strcpy(UserRecord.szLastMessage, "Account Created");
  
  
  pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));
  
  if (!pairInsert.second) {
    m_strLogMsg = "User: " + string(szUsername) + " NOT Added...Dupilcate Username ";
    CComLog::instance().log(m_strLogMsg, CComLog::Info);
    
    return USER_ALREADY_EXIST;
  }
  
  m_strLogMsg = "User: " + string(szUsername) + " Added ";
  CComLog::instance().log(m_strLogMsg, CComLog::Info);

  return VALID_USER;
}
//********************************************************************************************//
int CuserDB:: ChangeUserPassword(char* szUsername, char* szPassword)
{
  USER_RECORD UserRecord = {0};
  pair <MapUserDB::iterator, bool> pairInsert;
  
  m_itUserMap = m_UserMap.find(szUsername);
  
  if (m_itUserMap == m_UserMap.end()) {
    return INVALID_USER_NAME; 
  }

  UserRecord = m_itUserMap->second;
  strcpy(UserRecord.szPassword, szPassword);

  m_UserMap.erase(m_itUserMap);
  
  pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));

  m_strLogMsg = "User: " + string(szUsername);
  if (pairInsert.second){
    m_strLogMsg +=  " Password changed ";
    CComLog::instance().log(m_strLogMsg, CComLog::Info);
  }
  else {
    m_strLogMsg +=  " Error Modifiying... ";
    CComLog::instance().log(m_strLogMsg, CComLog::Error);
  }
   return true;
}
//********************************************************************************************//
int CuserDB::ModifyUser(char* szUsername, bool bActive, int iGroupID, int iAccessLevel)
{
  USER_RECORD UserRecord = {0};
  pair <MapUserDB::iterator, bool> pairInsert;
  
  m_itUserMap = m_UserMap.find(szUsername);
  
  if (m_itUserMap == m_UserMap.end()) {
    return INVALID_USER_NAME; 
  }

  UserRecord = m_itUserMap->second;
  
  UserRecord.bActive = bActive;
  UserRecord.iAccessLevel = iAccessLevel;
  UserRecord.iGroupID = iGroupID;
  

  m_UserMap.erase(m_itUserMap);
  
  pairInsert = m_UserMap.insert(pair<string, USER_RECORD> (szUsername, UserRecord));

  m_strLogMsg = "User: " + string(szUsername);
  if (pairInsert.second){
    m_strLogMsg +=  " Modified ";
    CComLog::instance().log(m_strLogMsg, CComLog::Info);
  }
  else {
    m_strLogMsg +=  " Error Modifiying... ";
    CComLog::instance().log(m_strLogMsg, CComLog::Error);
  }

  return true;
}
//********************************************************************************************//
int CuserDB::DeleteUser(char* szUsername)
{
  USER_RECORD UserRecord = {0};
  
  m_itUserMap = m_UserMap.find(szUsername);
  
  if (m_itUserMap == m_UserMap.end()) {
    return INVALID_USER_NAME; 
  }

  m_UserMap.erase(m_itUserMap);
  m_strLogMsg = "User: " + string(szUsername) + " Deleted ";
  CComLog::instance().log(m_strLogMsg, CComLog::Info);
  
  return true;
}
//********************************************************************************************//
int CuserDB::ListDB()
{
    int iWrite = 0;

    for (m_itUserMap = m_UserMap.begin(); m_itUserMap != m_UserMap.end(); ++m_itUserMap) {
        cout <<  m_itUserMap->second.szUserName;
        cout <<  m_itUserMap->second.szPassword;
        cout <<  m_itUserMap->second.iGroupID;
        cout <<  m_itUserMap->second.iAccessLevel;
        cout <<  m_itUserMap->second.bActive;
        cout <<  m_itUserMap->second.szPassword;
        cout <<  m_itUserMap->second.szLastLoginTime;
        cout <<  m_itUserMap->second.szLastLogoutTime;
        cout <<  m_itUserMap->second.szLastMessage;
        cout <<  m_itUserMap->second.szPassword;
        cout <<  m_itUserMap->second.szDateAccountCreated;
        cout <<  m_itUserMap->second.szDateAccountTerminated;
        cout <<  m_itUserMap->second.szLastMessage;
	
	iWrite++;
	cout<< "===============================================================================" << endl;
    }
    
    return true;
}
//********************************************************************************************//
int CuserDB::LoadUserFile()
{


    return true;
}
//********************************************************************************************//
int CuserDB::SaveUserFile()
{

    USER_RECORD UserRecord = {0};

    int iWrite;

    for (m_itUserMap = m_UserMap.begin(); m_itUserMap != m_UserMap.end(); ++m_itUserMap) {
        iWrite = write(m_ifd, &m_itUserMap->second, sizeof (UserRecord));
        if ( iWrite <= 0)
            break;
    }

    return true;
}
//********************************************************************************************//
