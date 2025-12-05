#include "GlobalKeyListener.h"
#include <cctype>
#include <algorithm>

std::vector<GlobalKeyListener*> GlobalKeyListener::s_instances;

GlobalKeyListener::GlobalKeyListener() : m_hKeyboardHook(nullptr) {
    registerInstance();
    startListening();
}

GlobalKeyListener::~GlobalKeyListener() {
    stopListening();
    unregisterInstance();
}

void GlobalKeyListener::registerInstance() {
    s_instances.push_back(this);
}

void GlobalKeyListener::unregisterInstance() {
    auto it = std::find(s_instances.begin(), s_instances.end(), this);
    if (it != s_instances.end()) {
        s_instances.erase(it);
    }
}

void GlobalKeyListener::addKeyEvent(char key, std::function<void()> handler) {
    char upperKey = std::toupper(key);
    
    auto it = m_keyHandlers.find(upperKey);
    if (it != m_keyHandlers.end()) {
        it->second.push_back(handler);
    } else {
        m_keyHandlers[upperKey] = {handler};
        m_keyStates[upperKey] = false;
    }
}

bool GlobalKeyListener::startListening() {
    if (m_hKeyboardHook) {
        return true;
    }
    
    m_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!m_hKeyboardHook) {
        return false;
    }
    return true;
}

void GlobalKeyListener::stopListening() {
    if (m_hKeyboardHook) {
        UnhookWindowsHookEx(m_hKeyboardHook);
        m_hKeyboardHook = nullptr;
    }
}

void GlobalKeyListener::clearAllHandlers() {
    m_keyHandlers.clear();
    m_keyStates.clear();
}

void GlobalKeyListener::removeKeyHandlers(char key) {
    char upperKey = std::toupper(key);
    auto it = m_keyHandlers.find(upperKey);
    if (it != m_keyHandlers.end()) {
        m_keyHandlers.erase(it);
        m_keyStates.erase(upperKey);
    }
}

LRESULT CALLBACK GlobalKeyListener::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    for (auto* instance : s_instances) {
        if (instance->isListening()) {
            instance->handleKeyboardEvent(nCode, wParam, lParam);
        }
    }
    
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT GlobalKeyListener::handleKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            char keyChar = virtualKeyToChar(pKeyStruct->vkCode);
            
            if (keyChar != 0) {
                char upperKey = std::toupper(keyChar);
                if (!m_keyStates[upperKey]) {
                    m_keyStates[upperKey] = true;
                    
                    executeHandlers(upperKey);
                }
            }
        }

        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            char keyChar = virtualKeyToChar(pKeyStruct->vkCode);
            if (keyChar != 0) {
                char upperKey = std::toupper(keyChar);
                m_keyStates[upperKey] = false;
            }
        }
    }
    
    return CallNextHookEx(m_hKeyboardHook, nCode, wParam, lParam);
}

void GlobalKeyListener::executeHandlers(char key) {
    auto it = m_keyHandlers.find(key);
    if (it != m_keyHandlers.end()) {
        
        for (auto& handler : it->second) {
            handler();
        }
    }
}

char GlobalKeyListener::virtualKeyToChar(DWORD vkCode) const {
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return static_cast<char>(vkCode);
    }
    if (vkCode >= '0' && vkCode <= '9') {
        return static_cast<char>(vkCode);
    }
    
    switch (vkCode) {
        case VK_SPACE: return ' ';
        case VK_OEM_PLUS: return '+';
        case VK_OEM_MINUS: return '-';
        case VK_OEM_PERIOD: return '.';
        case VK_OEM_COMMA: return ',';
        case VK_OEM_1: return ';';
        case VK_OEM_2: return '/';
        case VK_OEM_3: return '`';
        case VK_OEM_4: return '[';
        case VK_OEM_5: return '\\';
        case VK_OEM_6: return ']';
        case VK_OEM_7: return '\'';
        default: return 0;
    }
}