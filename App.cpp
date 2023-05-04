#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <deque>
#include "ProcessInfo.h"

using namespace std;

struct TokenInfo {
    string user;
    HANDLE token;
};

TokenInfo getToken(DWORD pid) {
    string userProcess;
    HANDLE cToken = NULL;
    HANDLE ph = NULL;
    ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, true, pid);
    if (ph == NULL) {
        cToken = (HANDLE)NULL;
    } else {
        BOOL res = OpenProcessToken(ph, MAXIMUM_ALLOWED, &cToken);
        if (!res) {
            cToken = (HANDLE)NULL;
        } else {
            DWORD dwSize = 0;
            GetTokenInformation(cToken, TokenUser, NULL, 0, &dwSize);
            PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
            if (GetTokenInformation(cToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
                SID_NAME_USE SidType;
                char lpName[MAX_PATH];
                DWORD dwNameSize = MAX_PATH;
                char lpDomain[MAX_PATH];
                DWORD dwDomainSize = MAX_PATH;
                if (LookupAccountSid(NULL, pTokenUser->User.Sid, lpName, &dwNameSize, lpDomain, &dwDomainSize, &SidType)) {
                    userProcess += lpDomain;
                    userProcess += "/";
                    userProcess += lpName;
                }
            }
            free(pTokenUser);
        }
    }
    if (ph != NULL) {
        CloseHandle(ph);
    }
    TokenInfo ti = TokenInfo();
    ti.user = userProcess;
    ti.token = cToken;
    return ti;
}

BOOL createProcess(HANDLE token, LPCWSTR app) {
    // initialize variables
    HANDLE dToken = NULL;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL res = TRUE;
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFOW);

    res = DuplicateTokenEx(token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &dToken);
    if (!res) {
        cout << "Error duplicating token" << endl;
        return res;
    }
    else {
        cout << "Token duplicated successfully" << endl;
    }
    res = CreateProcessWithTokenW(dToken, LOGON_WITH_PROFILE, app, NULL, 0, NULL, NULL, &si, &pi);
    if (!res) {
        cout << "Error creating process" << endl;
        return res;
    }
    else {
        cout << "Process created successfully" << endl;
    }
    return res;
}


int main(){
    deque<ProcessInfo> processes;
    TokenInfo ti;
    int pid = 0;
    bool infinte = true;
    string user;
    string userX;
    int userN = 0;
    bool result = false;
    bool duplicate = false;
    wstring app;
    string appStr;
    HANDLE hToken;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32); 
    if (Process32First(hSnap, &pe32) != FALSE){
        while (Process32Next(hSnap, &pe32) != FALSE){
            pid = pe32.th32ProcessID;
            ti = getToken(pid);
            user = ti.user;
            hToken = ti.token;
            ProcessInfo p = ProcessInfo(pid, user, hToken);
            duplicate = false; 
            for (int i = 0; i < processes.size(); i++){
                if (processes[i].getUser() == user || processes[i].getPid() == pid){
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate){
                processes.push_back(p);
            }
        }    
    }
    CloseHandle(hSnap);
    while(infinte){
        cout << endl << "With what user you want execute the new process? " << endl;
        for(int i = 0; i < processes.size(); i++){
            if(i == 0){}
            else{
                cout << i << ". " << processes[i].getUser() << endl;
            }
        }
        cout << "User: ";
        cin >> userN;
        userX = processes[userN].getUser();
        cout << "What process you want to execute? " << endl << "Process: ";
        cin >> appStr;
        app = wstring(appStr.begin(), appStr.end());
        LPCWSTR appLPCWSTR = app.c_str();

        cout << endl << "Process to execute: " << endl;
        cout << "User: " << userX << endl;
        cout << "Process: " << appStr << endl;

        result = createProcess(processes[userN].getToken(), appLPCWSTR);
    }
    return 0;
}