#include "HookMgr.h"
#include "include/MinHook.h"
#include "../Utils/Utils.h"

HookMgr* HookMgr::createInstance()
{
    if (MH_Initialize() != MH_OK) {
        return nullptr;
    }
    else {
        return new HookMgr();
    }
}

HookMgr::~HookMgr()
{
    MH_Uninitialize();
}


void* offset(HMODULE base_addr, uint64_t off) {
    return (void*)((uint64_t)base_addr + off);
}

void HookMgr::fastHook(HMODULE base_addr, uint64_t off, void* new_func, void** org_func) {
    void* addr = offset(base_addr, off);
    directHook(addr, new_func, org_func);
}

void HookMgr::fastHook(HMODULE base_addr, const char* func_name, void* new_func, void** org_func) {
    void* addr = Utils::get_proc_address(base_addr, func_name);
    directHook(addr, new_func, org_func);
}

void HookMgr::directHook(void* addr, void* new_func, void** org_func)
{
    MH_CreateHook(addr, new_func, org_func);
    MH_EnableHook(addr);
}
