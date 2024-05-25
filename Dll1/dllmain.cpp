// dllmain.cpp : 定义 DLL 应用程序的入口点。
#define DLLIMPORT __declspec(dllimport)
#include <Windows.h>
#include<stdio.h>
//包含Detour的头文件和库文件
#include "detours.h"
#pragma comment (lib,"detours.lib")

HANDLE hFile;
//保存函数原型（用指针存储要拦截的API）
LRESULT (__stdcall* OldSendMessageW)(HWND,UINT,WPARAM,LPARAM) = SendMessageW;

void NewSendMessageW(HWND hwnd,UINT Msg, WPARAM wParam, LPARAM lParam) {
	//OldGetLocalTime(lpSystemTime);
	char ClassName[512],WindowText[512],str[1024];
	
	GetClassName(hwnd,ClassName,sizeof(ClassName));
	GetWindowText(hwnd,WindowText,sizeof(WindowText));
	HANDLE handle=GetStdHandle(STD_OUTPUT_HANDLE);
	sprintf_s(str,sizeof(str),"\n\nSendMessageWCalled::::::\n");
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	//sprintf_s(str,sizeof(str),"Hwnd::  %x\n",hwnd);
	//WriteConsole(handle,str,strlen(str),NULL,NULL);
	//WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	sprintf_s(str,sizeof(str),"wintitle::  %s\n",WindowText);
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	sprintf_s(str,sizeof(str),"winClass::  %s\n",ClassName);
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	sprintf_s(str,sizeof(str),"Msg::  %u\n",Msg);
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	sprintf_s(str,sizeof(str),"wParam::  %u\n",wParam);
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	sprintf_s(str,sizeof(str),"lParam::  %u\n",lParam);
	WriteConsole(handle,str,strlen(str),NULL,NULL);
	WriteFile(hFile, str,strlen(str), NULL, NULL);
	
	OldSendMessageW(hwnd,Msg,wParam,lParam);
}



//下钩子函数
void StartHook() {
	//开始事务
	DetourTransactionBegin();
	//更新线程信息
	DetourUpdateThread(GetCurrentThread());
	//将拦截的函数附加到原函数的地址上
	DetourAttach(&(PVOID&)OldSendMessageW, NewSendMessageW);
	//DetourAttach(&(PVOID&)OldGetSystemTime, NewGetSystemTime);
	//结束事务
	DetourTransactionCommit();
}

//撤钩子函数
void EndHook() {
	//开始detours事务
	DetourTransactionBegin();
	//更新线程信息 
	DetourUpdateThread(GetCurrentThread());
	//将拦截的函数从原函数的地址上解除
	DetourDetach(&(PVOID&)OldSendMessageW, NewSendMessageW);
	//DetourDetach(&(PVOID&)OldGetSystemTime, NewGetSystemTime);
	//结束detours事务
	DetourTransactionCommit();
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH:{
	    AllocConsole();
	    hFile = CreateFile(
                "C:\\sendMessageW.log",
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
	    StartHook(); 
	    break;
	}
    case DLL_THREAD_ATTACH:break;
    case DLL_THREAD_DETACH:break;
    case DLL_PROCESS_DETACH:{
        EndHook(); 
        CloseHandle(hFile);
        break;
    }
    }
    return TRUE;
}

