#pragma once
/**
 * @file Random.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 24, 2026
 *
 * @brief RNG helpers.
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include <random>

namespace colloid
{

    /**
     * Generate a real random number between @p low and @p high.
     * @param low  The lower bound of the RNG. Default: 0.0
     * @param high The upper bound of the RNG. Default: 1.0
     * @return A random float between @p low and @p high.
     */
    inline float genRandom(const float low = 0.0f, const float high = 1.0f) noexcept
    {
        std::random_device rd;
        std::mt19937 generator(rd());

        std::uniform_real_distribution<float> realDist(low, high);

        return realDist(generator);
    }
} // namespace colloid
