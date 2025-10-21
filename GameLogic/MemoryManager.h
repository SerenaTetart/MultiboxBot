#pragma once
#include <functional>

class ThreadSynchronizer {
public:
    static void Init();
    static void RunOnMainThread(std::function<void()> action);

    static void pressKey(unsigned int key);
    static void releaseKey(unsigned int key);
};