#include <Threads/TimeWatch.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

TimeWatch::TimeWatch()
{
    m_LastTick = static_cast<double>(SDL_GetTicks64());
    m_CurTick = m_LastTick;
}

TimeWatch::~TimeWatch()
{
}

double TimeWatch::Tick()
{
    double newTick = static_cast<double>(SDL_GetTicks64());
    double delta = 1000.0 / m_TickRate - (newTick - m_LastTick);

    if (delta > 0) {
        int delayTicks = static_cast<int>(delta);
        SDL_Delay(delayTicks);
        newTick += delayTicks;
        delta -= delayTicks;
    }

    double dt = (newTick - m_CurTick) / 1000.0;
    m_LastTick = (delta < -30.0) ? newTick : newTick + delta;
    m_CurTick = newTick;

    return dt;
}

void TimeWatch::SetTickRate(double tickRate)
{
    m_TickRate = tickRate;
}