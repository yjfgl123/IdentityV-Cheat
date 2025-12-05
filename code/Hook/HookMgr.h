#pragma once
#include <windows.h>
#include <stdint.h>

class HookMgr {
public:
	static HookMgr* createInstance();
	~HookMgr();
	void fastHook(HMODULE base_addr, uint64_t off, void* new_func, void** org_func);
	void fastHook(HMODULE base_addr, const char* func_name, void* new_func, void** org_func);
	void directHook(void* addr, void* new_func, void** org_func);
};