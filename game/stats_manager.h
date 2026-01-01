#pragma once

#include <vector>
#include <cmath>
#include "common/logging.h"

class StatsManager
{
    public:
        StatsManager()
        {
            m_burstinessSamples.reserve(m_burstinessMaxSamples);
        }

        void pushBurstinessSample(int updatesThisFrame)
        {
            if (m_burstinessSamples.size() == m_burstinessMaxSamples)
            {
                m_burstinessSamples.erase(m_burstinessSamples.begin());
            }
            m_burstinessSamples.push_back(updatesThisFrame);
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

    private:
        static constexpr size_t m_burstinessMaxSamples = 60;
        std::vector<int> m_burstinessSamples;
};
