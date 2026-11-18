#pragma once
/**
 * @file Primitives.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 18, 2026
 *
 * @brief SIMD-accelerated rendering primitives like edge,
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include <falcon/simd/FalconSimd.h>

namespace colloid
{
    using Simd4x4 = flcn::Simd128_t<float, 4>;
    struct Edge
    {
        static constexpr size_t STEP_X_SIZE = 4;
        static constexpr size_t STEP_Y_SIZE = 1;

        Simd4x4 oneStepX;
        Simd4x4 oneStepY;

        Simd4x4 init(const flcn::CVec2F& v0, const flcn::CVec2F& v1, const flcn::CVec2F& origin)
        {
            // Edge setup
            const float a = v0.y() - v1.y();
            const float b = v1.x() - v0.x();
            const float c = v0.x() * v1.y() - v1.x() * v0.y();

            // Step deltas
            oneStepX = Simd4x4(a * STEP_X_SIZE);
            oneStepY = Simd4x4(b * STEP_Y_SIZE);

            // X/Y values for initial pixel block
            const auto x = Simd4x4(origin.x()) + Simd4x4(0, 1, 2, 3);
            const auto y = Simd4x4(origin.y());

            // Edge function value at origin
            return Simd4x4(a) * x + Simd4x4(b) * y + Simd4x4(c);
        }
    };
} // namespace colloid
