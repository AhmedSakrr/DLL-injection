// bInject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


/*----------------------------------
Changelog

0.1
    initial release
    
(c) by www.brickster.net
----------------------------------*/

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>
#include <fstream>

#include "ASCII.h"

// ����fLoadLibrary����ָ�룬�����ַ����õ�HINSTANCE
typedef HINSTANCE (__stdcall *fLoadLibrary)(char*);

// ����DLL������ַ����õ����̵ĵ�ַ?
typedef LPVOID (__stdcall *fGetProcAddress)(HINSTANCE, char*);

// fDLLjump DLL��ڣ�����ָ��
typedef void (*fDLLjump)(void);

// �ṹ�壬���������������Լ�Inject DLL��·�����������
struct INJECT
{
      fLoadLibrary LoadLibrary;
      fGetProcAddress GetProcAddress;
      char DLLpath[256];
      char DLLjump[16];
};

// ����ע��DLL��Ϣ��·������ں�����ע�����, �ƺ����ڱ�ע��Ľ�������ִ�еģ�
DWORD WINAPI InjectedCode(LPVOID addr)
{
	HINSTANCE hDll;
	fDLLjump DLLjump;
	INJECT* is = (INJECT*) addr;       
	hDll = is->LoadLibrary(is->DLLpath);
	DLLjump = (fDLLjump) is->GetProcAddress(hDll, is->DLLjump);
	// ����DLL ִ��DLLjump
	DLLjump();
	return 0;
}

// ��־λ��Ϊ��Ҫ��ô����
void InjectedEnd()
{
     /* This is to calculate the size of InjectedCode */
}

// ���ý��̵�DebugȨ��
void seDebugPrivilege()
{
	TOKEN_PRIVILEGES priv;
	HANDLE hThis, hToken;
	LUID luid;
	hThis = GetCurrentProcess();
	OpenProcessToken(hThis, TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue(0, L"seDebugPrivilege", &luid);
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Luid = luid;
	priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, false, &priv, 0, 0, 0);
	CloseHandle(hToken);
	CloseHandle(hThis);
}

// ���ݽ�����������Ӧ�Ľ���ID
DWORD lookupProgramID(const char process[]) {
      HANDLE hSnapshot;
      PROCESSENTRY32 ProcessEntry;
      ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
      hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   
      if (Process32First(hSnapshot, &ProcessEntry))
         do
         {
			DWORD dwNum = WideCharToMultiByte(CP_OEMCP,NULL,ProcessEntry.szExeFile,-1,NULL,0,NULL,FALSE);
			char *psText = new char[dwNum];
			if(!psText)
			{
				delete []psText;
			}
			
			WideCharToMultiByte (CP_OEMCP,NULL,ProcessEntry.szExeFile,-1,psText,dwNum,NULL,FALSE);
			printf("%s", psText);
          if (!strcmp(psText, process))
             {
              CloseHandle(hSnapshot);
              return ProcessEntry.th32ProcessID;
             }

		  delete []psText;
         } while(Process32Next(hSnapshot, &ProcessEntry));

      CloseHandle(hSnapshot);
      
      return 0;     
}

//
int main(int argc, char* argv[])
{
   HANDLE hProc;
   HINSTANCE hDll;
   LPVOID start, thread;
   DWORD codesize;
   INJECT is;
   
   // process name
   std::string pname;
   
   // print header
   ASCII::printBrickster();
   
   // enable debug privilege
   seDebugPrivilege();

   //MessageBox(NULL,L"MessageBoxText(����)",L"Title(����)",MB_OK);
   
   if (argc > 1) {
            pname = argv[1];
            
            hProc = OpenProcess(PROCESS_ALL_ACCESS, false, lookupProgramID(argv[1]));
   }
   else {
            std::cout << "\nPlease enter the name of the process (like program.exe): ";
            std::cin >> pname;

            hProc = OpenProcess(PROCESS_ALL_ACCESS, false, lookupProgramID(pname.c_str()));
   }
   
   if (hProc)
            std::cout << "\nProcess found and opened.";
   else {
            std::cout << "\nFailed opening process.\n";
            system("PAUSE");
            return 0;
   }

   if (argc > 2) 
            strcpy(is.DLLpath, argv[2]);
   else {
            std::string dll;
        
            std::cout << "\nPlease enter the path of the DLL: ";
            std::cin >> dll;
            
            strcpy(is.DLLpath, dll.c_str());
   }
   
   std::ifstream TestFile(is.DLLpath);
   
   if (TestFile)
            std::cout << "\nDLL file found.";
   else {
            std::cout << "\nDLL does not exist.\n";
            
            CloseHandle(hProc);
            
            system("PAUSE");
            return 0;
   }
   
   std::cout << "\nInjecting...";

   // ע��Ҫ�õ���kernel32.dll�е���������
   hDll = LoadLibrary(L"kernel32.dll");
   is.LoadLibrary = (fLoadLibrary) GetProcAddress(hDll, "LoadLibraryA");
   is.GetProcAddress = (fGetProcAddress) GetProcAddress(hDll, "GetProcAddress");
   
   // ���ÿ����ַ���
   strcpy(is.DLLjump, "DLLjump");
   
   // ����������ע������еĴ���Ƭ�δ�С
   codesize = (DWORD) InjectedEnd - (DWORD) InjectedCode;
   
   // �ڱ�ע������б����ڴ��С������INJECT�ṹ��Ĵ�С����ΪҪ�õ�
   start = VirtualAllocEx(hProc, 0, codesize + sizeof(INJECT), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
   // ��ע��������߳̿�ʼ��λ��, InjectedCode()
   thread = (LPVOID) ((DWORD) start + sizeof(INJECT));
   

   //�����뿽������ע�������ȥ����ʾ�ṹ�壬����thread ����
   /* Injecting the struct data first */
   WriteProcessMemory(hProc, start, (LPVOID) &is, sizeof(INJECT), NULL);

   /* Injecting the code */
   WriteProcessMemory(hProc, thread, (LPVOID) InjectedCode, codesize, NULL);

   std::cout << "\nCode has been injected.";
   
   std::cout << "\n\nProcess\n\t" << hProc << " (" << pname << ")\nSize of injected data\n\t" << sizeof(INJECT) << " bytes\nSize of injected code\n\t" << codesize << " bytes\nAt address\n\t" << start;
   
   std::cout << "\nDo you want to run the code now? (y/n) ";
   
   char a;
   
   std::cin >> a;
   
   switch (a) {
          case 'y':
               CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE) thread, start, 0, 0);
               std::cout << "\nThread started! Code is now up and running.\n";
               break;
          case 'n':
          default:
               std::cout << "\nTerminated.\n";
               break;
   }

   CloseHandle(hProc);
   
   system("PAUSE");
   
   return 0;
}



