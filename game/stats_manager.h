#pragma once

#include <map>
#include <deque>
#include <cmath>

#include "common/vector_2d.h"
#include "player.h"

class StatsManager
{
    public:
        StatsManager() = default;

        void pushBurstinessSample(int updatesThisFrame)
        {
            updateRollingBuffer(m_burstinessSamples, updatesThisFrame, m_burstinessMaxSamples);
        }

        double computeBurstinessCV()
        {
            if (m_burstinessSamples.empty())
            {
                return 0.0;
            }

            double mean = 0.0;
            for (const auto& updates : m_burstinessSamples)
            {
                mean += updates;
            }
            mean /= m_burstinessSamples.size();

            double variance = 0.0;
            for (const auto& updates : m_burstinessSamples)
            {
                double diff = updates - mean;
                variance += diff * diff;
            }
            variance /= m_burstinessSamples.size();

            double stddev = std::sqrt(variance);
            double cv = (mean != 0.0) ? stddev / mean : 0.0;

            return cv;
        }

        void pushRemoteMotionSample(PlayerID playerID, Vector2D<float> currentPosition)
        {
            auto posIt = m_previousRemotePositions.find(playerID);
            if (posIt == m_previousRemotePositions.end()) {
                m_previousRemotePositions[playerID] = currentPosition;
                return;
            }

            Vector2D<float> previousPosition = posIt->second;
            Vector2D<float> currentDelta = currentPosition - previousPosition;

            auto deltaIt = m_previousRemoteDeltas.find(playerID);
            if (deltaIt == m_previousRemoteDeltas.end()) {
                m_previousRemotePositions[playerID] = currentPosition;
                m_previousRemoteDeltas[playerID] = currentDelta;
                return;
            }

            Vector2D<float> predictedPosition = previousPosition + deltaIt->second;

            Vector2D<float> error = currentPosition - predictedPosition;
            double jitter = std::hypot(error.x, error.y);

            posIt->second = currentPosition;
            deltaIt->second = currentDelta;

            updateRollingBuffer(m_remoteJitterSamples, jitter, m_remoteJitterMaxSamples);
        }

        double computeRemoteMotionJitter()
        {
            if (m_remoteJitterSamples.empty()) return 0.0;

            double sum = 0.0;
            for (double jitter : m_remoteJitterSamples) sum += jitter;
            return sum / m_remoteJitterSamples.size();
        }

    private:
        template <typename T>
        void updateRollingBuffer(std::deque<T>& buffer, T value, size_t maxSamples)
        {
            buffer.push_back(value);
            if (buffer.size() > maxSamples) {
                buffer.pop_front();
            }
        }

    private:
        static constexpr size_t m_burstinessMaxSamples = 60;
        std::deque<int> m_burstinessSamples;

        static constexpr size_t m_remoteJitterMaxSamples = 60;
        std::deque<double> m_remoteJitterSamples;

        std::map<PlayerID, Vector2D<float>> m_previousRemotePositions;
        std::map<PlayerID, Vector2D<float>> m_previousRemoteDeltas;
};
