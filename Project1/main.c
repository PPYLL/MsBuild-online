#include<windows.h>
#include<stdio.h>


/*判断系统架构，并定义ZwCreateThreadEx函数指针*/
#ifdef _WIN64
typedef DWORD(WINAPI* pZwCreateThreadEx)(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	LPVOID ObjectAttributes,
	HANDLE ProcessHandle,
	LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter,
	ULONG CreateThreadFlags,
	SIZE_T ZeroBits,
	SIZE_T StackSize,
	SIZE_T MaximumStackSize,
	LPVOID pUnkown
	);
#else
typedef DWORD(WINAPI* pZwCreateThreadEx)(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess,
	LPVOID ObjectAttributes,
	HANDLE ProcessHandle,
	LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter,
	BOOL CreateSuspended,
	DWORD dwStackSize,
	DWORD dw1,
	DWORD dw2,
	LPVOID pUnkown
	);
#endif


/*
设定本进程的程序调试权限
lPcstr:权限字符串
backCode:错误返回码
*/
BOOL GetDebugPrivilege(
	LPCSTR lPcstr,
	DWORD* backCode
)
{
	HANDLE Token = NULL;
	LUID luid = { 0 };
	TOKEN_PRIVILEGES Token_privileges = { 0 };
	//内存初始化为zero
	memset(&luid, 0x00, sizeof(luid));
	memset(&Token_privileges, 0x00, sizeof(Token_privileges));

	//打开进程令牌
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &Token))
	{
		*backCode = 0x01;
		return FALSE;
	}

	//获取特权luid
	if (!LookupPrivilegeValue(NULL, lPcstr, &luid))
	{
		*backCode = 0x02;
		return FALSE;
	}

	//设定结构体luid与特权
	Token_privileges.PrivilegeCount = 1;
	Token_privileges.Privileges[0].Luid = luid;
	Token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//修改进程特权
	if (!AdjustTokenPrivileges(Token, FALSE, &Token_privileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		*backCode = 0x03;
		return FALSE;
	}
	*backCode = 0x00;
	return TRUE;
}

void inject(DWORD pid) {
	char dllpath[] = "Hook.dll";
	DWORD backCode = 0;
	HANDLE hProcess = NULL;
	LPVOID Buff = NULL;
	//HMODULE Ntdll = NULL;
	SIZE_T write_len = 0;
	DWORD dwStatus = 0;
	HANDLE hRemoteThread = NULL;



	//提升进程特权，获得调试权限
	if (!GetDebugPrivilege(SE_DEBUG_NAME, &backCode))
	{
		puts("DBG privilege error");
		printf(" %d", backCode);
		return 0;
	}

	//打开要被注入的进程
	if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL)
	{
		puts("process open erro");
		return 0;
	}

	//在要被注入的进程中创建内存，用于存放注入dll的路径
	Buff = VirtualAllocEx(hProcess, NULL, strlen(dllpath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (Buff == NULL)
	{
		puts("Buff alloc error");
		return 0;
	}

	//将dll路径写入刚刚创建的内存中
	WriteProcessMemory(hProcess, Buff, dllpath, strlen(dllpath) + 1, &write_len);
	if (strlen(dllpath) + 1 != write_len)
	{
		puts("write error");
		getchar();
	}

	//加载ntdll.dll并从中获取内核函数ZwCreateThread，并使用函数指针指向此函数

	pZwCreateThreadEx ZwCreateThreadEx =
		(pZwCreateThreadEx)GetProcAddress(LoadLibrary("ntdll.dll"), "ZwCreateThreadEx");
	if (ZwCreateThreadEx == NULL)
	{
		puts("func get error");
		return 0;
	}
	//执行ZwCreateThread函数，在指定进程中创建线程加载要被注入的dll
	dwStatus = ZwCreateThreadEx(
		&hRemoteThread,
		PROCESS_ALL_ACCESS,
		NULL,
		hProcess,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
		Buff,
		0, 0, 0, 0,
		NULL
	);
	if (hRemoteThread == NULL)
	{
		puts("zwcreatethread fun error");
		return 0;
	}
	//puts(shellcode);
	//printf("%d\n", strlen(shellcode));
	//释放不需要的变量以及内存
	CloseHandle(hProcess);
}

int main() {
	PROCESS_INFORMATION lpProcessInformation;
	STARTUPINFOA lpStartupInfo;
	ZeroMemory(&lpStartupInfo, sizeof(lpStartupInfo));
	lpStartupInfo.cb = sizeof(lpStartupInfo);
	CreateProcessA(NULL, "PPTService.exe",NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,NULL,&lpStartupInfo,&lpProcessInformation);
	printf("%d\n", lpProcessInformation.dwProcessId);
	inject(lpProcessInformation.dwProcessId);
	ResumeThread(lpProcessInformation.hThread);
	getchar();
}