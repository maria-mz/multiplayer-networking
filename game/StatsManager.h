#pragma once

#include <vector>
#include <cmath>
#include "../common/Logging.h"

class StatsManager
{
    public:
        StatsManager()
        {
            m_updatesPerFrameSamples.reserve(m_updatesPerFrameMaxSamples);
        }

        void pushUpdatesPerFrameSample(int updatesThisFrame)
        {
            if (m_updatesPerFrameSamples.size() == m_updatesPerFrameMaxSamples)
            {
                m_updatesPerFrameSamples.erase(m_updatesPerFrameSamples.begin());
            }
            m_updatesPerFrameSamples.push_back(updatesThisFrame);
        }

        double computeUpdatesPerFrameCV()
        {
            if (m_updatesPerFrameSamples.empty())
            {
                return 0.0;
            }

            double mean = 0.0;
            for (const auto& updates : m_updatesPerFrameSamples)
            {
                mean += updates;
            }
            mean /= m_updatesPerFrameSamples.size();

            double variance = 0.0;
            for (const auto& updates : m_updatesPerFrameSamples)
            {
                double diff = updates - mean;
                variance += diff * diff;
            }
            variance /= m_updatesPerFrameSamples.size();

            double stddev = std::sqrt(variance);
            double cv = (mean != 0.0) ? stddev / mean : 0.0;

            return cv;
        }

    private:
        static constexpr size_t m_updatesPerFrameMaxSamples = 60;
        std::vector<int> m_updatesPerFrameSamples;
};
