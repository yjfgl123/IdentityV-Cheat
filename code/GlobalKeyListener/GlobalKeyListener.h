#pragma once

#include <windows.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>

class GlobalKeyListener {
private:
    HHOOK m_hKeyboardHook;
    
    // 存储按键事件处理函数：key -> 处理函数列表
    std::unordered_map<char, std::vector<std::function<void()>>> m_keyHandlers;
    
    // 当前按下的按键状态
    std::unordered_map<char, bool> m_keyStates;
    
    // 实例特定的钩子回调包装器
    static std::vector<GlobalKeyListener*> s_instances;
    
public:
    GlobalKeyListener();
    bool startListening();
    ~GlobalKeyListener();
    
    GlobalKeyListener(const GlobalKeyListener&) = delete;
    GlobalKeyListener& operator=(const GlobalKeyListener&) = delete;
    
    void addKeyEvent(char key, std::function<void()> handler);
    
    
    bool isListening() const { return m_hKeyboardHook != nullptr; }
    
    void clearAllHandlers();
    
    void removeKeyHandlers(char key);
    
private:

    void stopListening();

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    LRESULT handleKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);
    
    void executeHandlers(char key);
    
    char virtualKeyToChar(DWORD vkCode) const;
    
    void registerInstance();
    void unregisterInstance();
};