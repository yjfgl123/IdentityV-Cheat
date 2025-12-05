#include "mem.h"
#include <winsvc.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")


namespace MemoryInternal {
	BOOL IsCurrentProcessExe(LPCWSTR exeName)
	{
		WCHAR currentPath[MAX_PATH];
		if (GetModuleFileNameW(NULL, currentPath, MAX_PATH) == 0) {
			return FALSE;
		}
		WCHAR* fileName = wcsrchr(currentPath, L'\\');
		if (fileName == NULL) {
			fileName = currentPath;
		}
		else {
			fileName++;
		}
		return _wcsicmp(fileName, exeName) == 0;
	}

	bool read_memory(void* dst, uint64_t size, uint64_t address)
	{
		__try {
			memcpy(dst, (const void*)(address), size);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}
	bool write_memory(uint64_t address,const void* buffer, uint64_t size) {
		__try {
			memcpy((void*)address, buffer, size);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	bool attach_process()
	{
		return true;
	}

	void detach_process()
	{

	}

	bool init(LPCWSTR process)
	{
		if (IsCurrentProcessExe(process)) {
			return true;
		}
		else {
			return false;
		}
	}

	Memory::MemoryPointer get_module_base(LPCWSTR dll_name) {
		return Memory::MemoryPointer((uint64_t)GetModuleHandle(dll_name));
	}
}

#define CACHE_SIZE 512
#define CACHE_OBJ_SIZE 4096

namespace MemoryR3 {
	LPCWSTR m_process = NULL;

	DWORD GetProcessIdByName(LPCWSTR processName) {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return 0;
		}
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe)) {
			do {
				if (wcscmp(pe.szExeFile, processName) == 0) {
					CloseHandle(hSnapshot);
					return pe.th32ProcessID;
				}
			} while (Process32Next(hSnapshot, &pe));
		}
		CloseHandle(hSnapshot);
		return 0;
	}


	HANDLE hProcess = NULL;

	bool init(LPCWSTR process) {
		m_process = process;
		return true;
	}
	bool read_memory(void* dst, uint64_t size, uint64_t address)
	{
		uint64_t bytes_read;
		BOOL success = ReadProcessMemory(hProcess, (LPCVOID)address, dst, size, &bytes_read);
		if (success == FALSE || bytes_read != size) {
			return false;
		}
		else {
			return true;
		}
	}
	bool write_memory(uint64_t address,const void* buffer, uint64_t size) {
		uint64_t bytes_write;
		BOOL success = WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytes_write);
		if (success == FALSE || bytes_write != size) {
			return false;
		}
		else {
			return true;
		}
	}

	bool attach_process()
	{
		DWORD pid = GetProcessIdByName(m_process);
		hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid);
		if (hProcess) {
			return true;
		}
		else {
			return false;
		}
	}

	void detach_process()
	{
		CloseHandle(hProcess);
	}
	Memory::MemoryPointer get_module_base(LPCWSTR dll_name) {
		HMODULE hModules[1024];
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
			for (DWORD i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
				wchar_t szModuleName[MAX_PATH];
				if (GetModuleFileNameExW(hProcess, hModules[i], szModuleName, MAX_PATH)) {
					wchar_t* fileName = wcsrchr(szModuleName, L'\\');
					fileName = fileName ? fileName + 1 : szModuleName;

					if (_wcsicmp(fileName, dll_name) == 0) {
						MODULEINFO moduleInfo;
						if (GetModuleInformation(hProcess, hModules[i], &moduleInfo, sizeof(moduleInfo))) {
							return Memory::MemoryPointer((uint64_t)hModules[i]);
						}
					}
				}
			}
		}
		return Memory::MemoryPointer(0);
	}
}

namespace Memory {

	bool read_memory(void* dst, uint64_t size, uint64_t address)
	{
		return MemoryLib::read_memory(dst,size,address);
	}
	bool write_memory(uint64_t address, const void* buffer, uint64_t size)
	{
		return MemoryLib::write_memory(address,buffer,size);
	}
	bool attach_process()
	{
		return MemoryLib::attach_process();
	}

	void detach_process()
	{
		MemoryLib::detach_process();
	}
	bool init(LPCWSTR process)
	{
		return MemoryLib::init(process);
	}

	MemoryPointer get_module_base(LPCWSTR dll_name) {
		return MemoryLib::get_module_base(dll_name);
	}
}


