#ifndef __THREAD_H_
#define __THREAD_H_

#include <functional>
#include <vector>
#include <thread>
#include "TimeWatch.h"

class Thread
{
public:
    Thread();
    Thread(std::function<void(double)> callback, double tickRate = 1.0);
    Thread(
        std::function<void(void)> preCallback,
        std::function<void(double)> onCallback,
        std::function<void(void)> posCallback,
        double tickRate = 1.0
    );

    void Start();
    void Stop();

    void SetTickRate(double tickRate);

    void Tick();
    std::thread::id GetId();

    void AddCallback(std::function<void(void)>);

private:
    std::function<void(double)> m_ThreadFunction;
    std::function<void(void)> m_ThreadPreFunction;
    std::function<void(void)> m_ThreadPosFunction;

    std::vector<std::function<void(void)>> m_Callback;

    bool m_Running;
    std::thread m_Thread;
    std::thread::id m_Id;
    TimeWatch m_TimeWatch;
};

#endif