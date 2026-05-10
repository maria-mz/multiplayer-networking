#pragma once

#include <deque>
#include <cmath>

class StatsManager
{
    public:
        StatsManager() = default;

        void pushRemoteUpdateVariabilitySample(int updatesThisFrame)
        {
            updateRollingBuffer(
                m_remoteUpdateVariabilitySamples,
                updatesThisFrame,
                m_remoteUpdateVariabilityMaxSamples
            );
        }

        double computeRemoteUpdateVariability()
        {
            if (m_remoteUpdateVariabilitySamples.empty())
            {
                return 0.0;
            }

            double mean = 0.0;
            for (const auto& updates : m_remoteUpdateVariabilitySamples)
            {
                mean += updates;
            }
            mean /= m_remoteUpdateVariabilitySamples.size();

            double variance = 0.0;
            for (const auto& updates : m_remoteUpdateVariabilitySamples)
            {
                double diff = updates - mean;
                variance += diff * diff;
            }
            variance /= m_remoteUpdateVariabilitySamples.size();

            double stddev = std::sqrt(variance);
            double cv = (mean != 0.0) ? stddev / mean : 0.0;

            return cv;
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
        static constexpr size_t m_remoteUpdateVariabilityMaxSamples = 60;
        std::deque<int> m_remoteUpdateVariabilitySamples;
};
