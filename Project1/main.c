#include<windows.h>
#include<stdio.h>


/*�ж�ϵͳ�ܹ���������ZwCreateThreadEx����ָ��*/
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
�趨�����̵ĳ������Ȩ��
lPcstr:Ȩ���ַ���
backCode:���󷵻���
*/
BOOL GetDebugPrivilege(
	LPCSTR lPcstr,
	DWORD* backCode
)
{
	HANDLE Token = NULL;
	LUID luid = { 0 };
	TOKEN_PRIVILEGES Token_privileges = { 0 };
	//�ڴ��ʼ��Ϊzero
	memset(&luid, 0x00, sizeof(luid));
	memset(&Token_privileges, 0x00, sizeof(Token_privileges));

	//�򿪽�������
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &Token))
	{
		*backCode = 0x01;
		return FALSE;
	}

	//��ȡ��Ȩluid
	if (!LookupPrivilegeValue(NULL, lPcstr, &luid))
	{
		*backCode = 0x02;
		return FALSE;
	}

	//�趨�ṹ��luid����Ȩ
	Token_privileges.PrivilegeCount = 1;
	Token_privileges.Privileges[0].Luid = luid;
	Token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//�޸Ľ�����Ȩ
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



	//����������Ȩ����õ���Ȩ��
	if (!GetDebugPrivilege(SE_DEBUG_NAME, &backCode))
	{
		puts("DBG privilege error");
		printf(" %d", backCode);
		return 0;
	}

	//��Ҫ��ע��Ľ���
	if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL)
	{
		puts("process open erro");
		return 0;
	}

	//��Ҫ��ע��Ľ����д����ڴ棬���ڴ��ע��dll��·��
	Buff = VirtualAllocEx(hProcess, NULL, strlen(dllpath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (Buff == NULL)
	{
		puts("Buff alloc error");
		return 0;
	}

	//��dll·��д��ոմ������ڴ���
	WriteProcessMemory(hProcess, Buff, dllpath, strlen(dllpath) + 1, &write_len);
	if (strlen(dllpath) + 1 != write_len)
	{
		puts("write error");
		getchar();
	}

	//����ntdll.dll�����л�ȡ�ں˺���ZwCreateThread����ʹ�ú���ָ��ָ��˺���

	pZwCreateThreadEx ZwCreateThreadEx =
		(pZwCreateThreadEx)GetProcAddress(LoadLibrary("ntdll.dll"), "ZwCreateThreadEx");
	if (ZwCreateThreadEx == NULL)
	{
		puts("func get error");
		return 0;
	}
	//ִ��ZwCreateThread��������ָ�������д����̼߳���Ҫ��ע���dll
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
	//�ͷŲ���Ҫ�ı����Լ��ڴ�
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