#ifndef FRAME_TIMER_H_
#define FRAME_TIMER_H_

#include <SDL2/SDL.h>

class FrameTimer {
    public:
        FrameTimer(int targetFrameDuration)
            : m_targetFrameDuration(targetFrameDuration)
            , m_lastTick(SDL_GetTicks()), m_startTick(0) {}

        void startFrame() {
            m_startTick = SDL_GetTicks();
            m_deltaTime = m_startTick - m_lastTick;
            m_lastTick = m_startTick;
        }

        void endFrame() {
            int frameDuration = SDL_GetTicks() - m_startTick;
            if (frameDuration < m_targetFrameDuration) {
                SDL_Delay(m_targetFrameDuration - frameDuration);
            }
        }

        int getDeltaTime() const {
            return m_deltaTime;
        }

    private:
        int m_targetFrameDuration;
        int m_lastTick;
        int m_startTick;
        int m_deltaTime;
};

#endif