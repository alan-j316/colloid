/**
 * @file Renderer.cpp
 * @author Alan Abraham P Kochumon
 * @date Created on: November 09, 2026
 *
 * @brief Implementation of member function declared in Rasterizer.h
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include "Renderer.h"

#include "Mesh.h"
#include "Primitives.h"

#include <algorithm>

namespace rasterizer
{
    template <typename T>
    void Renderer::renderTriangleWireframe(const flcn::CVec2<T>& v0, const flcn::CVec2<T>& v1, const flcn::CVec2<T>& v2)
    {
        renderLine(v0.x(), v0.y(), v1.x(), v1.y(), 0xff, 0xff, 0xff, 0xff);
        renderLine(v1.x(), v1.y(), v2.x(), v2.y(), 0xff, 0xff, 0xff, 0xff);
        renderLine(v2.x(), v2.y(), v0.x(), v0.y(), 0xff, 0xff, 0xff, 0xff);
    }


    template <typename T>
    T Renderer::edgeCross(const flcn::CVec2<T>& v0, const flcn::CVec2<T>& v1, const flcn::CVec2<T>& p)
    {
        // We are assuming that the edges start from vert0
        const auto edge0 = v1 - v0;
        const auto edge1 = p - v0;
        return edge0.cross(edge1);
    }


    BoundingBox getBounds(const flcn::Vec3F& v0, const flcn::Vec3F& v1, const flcn::Vec3F& v2) noexcept
    {
        return BoundingBox{
            .top    = static_cast<uint32_t>(std::min({ v0.y(), v1.y(), v2.y() })),
            .left   = static_cast<uint32_t>(std::min({ v0.x(), v1.x(), v2.x() })),
            .right  = static_cast<uint32_t>(std::max({ v0.x(), v1.x(), v2.x() })),
            .bottom = static_cast<uint32_t>(std::max({ v0.y(), v1.y(), v2.y() })),
        };
    }

    void Renderer::renderTriangle(const flcn::Vec3F& v0, const flcn::Vec3F& v1, const flcn::Vec3F& v2, const uint8_t r,
                                  const uint8_t g, const uint8_t b, const uint8_t a) noexcept
    {
        const auto projV0 = v0.template swizzle<flcn::axis::X, flcn::axis::Y>();
        const auto projV1 = v1.template swizzle<flcn::axis::X, flcn::axis::Y>();
        const auto projV2 = v2.template swizzle<flcn::axis::X, flcn::axis::Y>();


        // Calculate the bounding box of the triangle
        // (minX, minY)--------
        //       |____________|
        //       |\ \ \ \ \ \/|
        //       | \ \ \ \ \/ |
        //       |  \ \ \ \/  |
        //       |   \ \ \/   |
        //       |    \ \/    |
        //       |     \/     |
        //       --------(maxX, maxY)
        auto bb = getBounds(v0, v1, v2);

        // Clip against screen bounds
        bb.left   = std::max(bb.left, 0u);
        bb.top    = std::max(bb.top, 0u);
        bb.right  = std::min(bb.right, static_cast<uint32_t>(width - 1));
        bb.bottom = std::min(bb.bottom, static_cast<uint32_t>(height - 1));


        // Cross between two edges, v0v1(v1 - v0) and v0v2(v2 - v0)
        const auto triArea = (projV1 - projV0).cross(projV2 - projV0) * 0.5f;

        // Backface culling
        if (triArea < 0.0f) // Triangle is inverted or less than 1 px
        {
            return;
        }

        // Compute top left for each edge (vertex pair)
        const auto isTopLeft0 = isTopLeftEdge(projV0, projV1);
        const auto isTopLeft1 = isTopLeftEdge(projV1, projV2);
        const auto isTopLeft2 = isTopLeftEdge(projV2, projV0);

        const auto bias0 = isTopLeft0 * EPSILON;
        const auto bias1 = isTopLeft1 * EPSILON;
        const auto bias2 = isTopLeft2 * EPSILON;

        // Calculate the barycenter coordinate for the first point
        // F01(P) = (a_y - b_y) * Px + (b_x - a_x) * Py + (a_x * b_y - a_y * b_x)
        // F01(P) =        A_01 * Px +        B_01 * Py + C01
        // Permutation F01, F12, F20
        const auto a01 = v0.y() - v1.y();
        const auto b01 = v1.x() - v0.x();
        const auto a12 = v1.y() - v2.y();
        const auto b12 = v2.x() - v1.x();
        const auto a20 = v2.y() - v0.y();
        const auto b20 = v0.x() - v2.x();

        const auto p0 = flcn::CVec2F(static_cast<float>(bb.left), static_cast<float>(bb.top));
        auto w0Row    = edgeCross(projV0, projV1, p0);
        auto w1Row    = edgeCross(projV1, projV2, p0);
        auto w2Row    = edgeCross(projV2, projV0, p0);


        // Looping in reverse order for better cache locality
        for (auto y = bb.top; y <= bb.bottom; ++y)
        {
            auto w0 = w0Row;
            auto w1 = w1Row;
            auto w2 = w2Row;
            for (auto x = bb.left; x <= bb.right; ++x)
            {
                const auto offset      = y * static_cast<size_t>(width) + x;
                const auto colorOffset = static_cast<size_t>(colorChannels) * offset;

                if (offset > static_cast<uint32_t>(width * height))
                {
                    continue;
                }

                const auto currentPixelDepth = (w0 * v2.z() + w1 * v0.z() + w2 * v1.z()) / (w0 + w1 + w2);


                // A comparison of anything other than 0.0f, like 1e-5 or 1e-10 will cause visual glitches
                const bool eC0 = w0 - bias0 >= 0.0f;
                const bool eC1 = w1 - bias1 >= 0.0f;
                const bool eC2 = w2 - bias2 >= 0.0f;
                // if (w0 > 0.0 && w1 > 0.0 && w2 > 0.0)
                // {
                //     std::cout << std::format("w0 {} w1 {} w2 {}\n", w0, w1, w2);
                // }

                // putPixel(colorOffset, r, g, b, a);
                // auto w02    = edgeCross(projV1, projV2, point);
                // auto w12    = edgeCross(projV2, projV0, point);
                // auto w22    = edgeCross(projV0, projV2, point);
                // Edge cross && Depth buffer-based culling
                // std::cout << "Rendering: Excluded<" << currentPixelDepth << ">\n";
                if ((std::bit_cast<int32_t>(w0) | std::bit_cast<int32_t>(w1) | std::bit_cast<int32_t>(w2)) >= 0 &&
                    currentPixelDepth >= depthBuffer[offset])
                // if (eC0 && eC1 && eC2 && currentPixelDepth > depthBuffer[offset])
                {
                    // Color Rendering
                    putPixel(colorOffset, r, g, b, a);
                    // std::cout << "Rendering: Depth<" << currentPixelDepth << ">\n";

                    // Update depth buffer
                    depthBuffer[offset] = currentPixelDepth;

                    // Depth buffer rendering
                    // const auto depthColor = static_cast<uint8_t>(depthBuffer[offset] * 50);
                    // putPixel(colorOffset, depthColor, depthColor, depthColor);
                }

                w0 += a01;
                w1 += a12;
                w2 += a20;
            }
            w0Row += b01;
            w1Row += b12;
            w2Row += b20;
        }
    }



    void Renderer::renderLine(const float ax, const float ay, const float bx, const float by, const uint8_t r,
                              const uint8_t g, const uint8_t b, const uint8_t a) const noexcept
    {
        for (float t = 0.0f; t <= 1.0f; t += 0.002f)
        {
            const auto x            = static_cast<int>(std::round(ax + t * (bx - ax)));
            const auto y            = static_cast<int>(std::round(ay + t * (by - ay)));
            const auto bufferOffset = colorChannels * (width * y + x);
            if constexpr (std::endian::native == std::endian::little)
            {
                frameBuffer[bufferOffset + 0] = b;
                frameBuffer[bufferOffset + 1] = g;
                frameBuffer[bufferOffset + 2] = r;
                frameBuffer[bufferOffset + 3] = a;
            }
            else if constexpr (std::endian::native == std::endian::big)
            {
                frameBuffer[bufferOffset + 0] = r;
                frameBuffer[bufferOffset + 1] = g;
                frameBuffer[bufferOffset + 2] = b;
                frameBuffer[bufferOffset + 3] = a;
            }
        }
    }


    void Renderer::clearScreen() const noexcept
    {
        std::memset(frameBuffer, clearColor, static_cast<size_t>(width * height * colorChannels));
        // Clear the depth buffer
        std::memset(depthBuffer, 0, static_cast<size_t>(width * height * 4));
    }

    Renderer::~Renderer() noexcept
    {
        delete[] frameBuffer;
        delete[] depthBuffer;
    }

    BarycentricCoordinates Renderer::computeBaryCentricCoordinates(const flcn::Vec2F& v0, const flcn::Vec2F& v1,
                                                                   const flcn::Vec2F& v2,
                                                                   const flcn::Vec2F& point) const noexcept
    {
        // Division by two is omitted since that is common in smaller triangles area too
        const auto triArea   = (v1 - v0).cross(v2 - v1);
        const auto alphaArea = (v2 - v1).cross(point - v1);
        const auto betaArea  = (v0 - v2).cross(point - v2);
        const auto gammaArea = (v1 - v0).cross(point - v0);

        return BarycentricCoordinates{ .alpha = alphaArea / triArea,
                                       .beta  = betaArea / triArea,
                                       .gamma = gammaArea / triArea };
    }


    BarycentricCoordinates Renderer::computeBaryCentricCoordinates(const flcn::CVec2F& v0, const flcn::CVec2F& v1,
                                                                   const flcn::CVec2F& v2, const flcn::CVec2F& point,
                                                                   const float triArea) noexcept
    {
        const auto alphaArea = 0.5f * (v2 - v1).cross(point - v1);
        const auto betaArea  = 0.5f * (v0 - v2).cross(point - v2);
        const auto gammaArea = 0.5f * (v1 - v0).cross(point - v0);

        return BarycentricCoordinates{ .alpha = alphaArea / triArea,
                                       .beta  = betaArea / triArea,
                                       .gamma = gammaArea / triArea };
    }


    bool Renderer::isTopLeftEdge(const flcn::CVec2F& v0, const flcn::CVec2F& v1) noexcept
    {
        const auto edge = v1 - v0;
        // Top Left if edge's x coordinate is positive (negative is bottom edge due to clockwise convention
        // used) and if y coordinate is 0
        const auto isTopEdge = edge.x() >= EPSILON && std::abs(edge.y()) < EPSILON;
        // Left Edge if y coordinate is >=0
        const auto isLeftEdge = edge.y() >= 1e-5;

        return isTopEdge && isLeftEdge;
    }


    void Renderer::putPixel(const size_t bufferOffset, const uint8_t r, const uint8_t g, const uint8_t b,
                            const uint8_t a) const noexcept
    {

        if constexpr (std::endian::native == std::endian::big)
        {
            // ARGB
            frameBuffer[bufferOffset]     = r;
            frameBuffer[bufferOffset + 1] = g;
            frameBuffer[bufferOffset + 2] = b;
            frameBuffer[bufferOffset + 3] = a;
        }
        else if constexpr (std::endian::native == std::endian::little)
        {
            // BGRA
            frameBuffer[bufferOffset]     = b;
            frameBuffer[bufferOffset + 1] = g;
            frameBuffer[bufferOffset + 2] = r;
            frameBuffer[bufferOffset + 3] = a;
        }
    }


    BoundingBox Renderer::computeBoundingBox(const flcn::CVec2F& v0, const flcn::CVec2F& v1,
                                             const flcn::CVec2F& v2) noexcept
    {
        return BoundingBox{
            .top    = static_cast<uint32_t>(std::min({ v0.y(), v1.y(), v2.y() })),
            .left   = static_cast<uint32_t>(std::min({ v0.x(), v1.x(), v2.x() })),
            .right  = static_cast<uint32_t>(std::max({ v0.x(), v1.x(), v2.x() })),
            .bottom = static_cast<uint32_t>(std::max({ v0.y(), v1.y(), v2.y() })),
        };
    }

    static float rot = 0.0f;
    void Renderer::render(const Mesh& mesh, const float deg) noexcept
    {
        std::vector<flcn::Vec3<float>> vertices(mesh.vertices.begin(), mesh.vertices.end());

        auto view  = flcn::Mat4F::identity();
        view(2, 3) = -3;
        // NOTE: The near and far planes are switched to ensure higher precision for the depth testing as
        //       with opposite order almost all the values are clumped near the 0.97 range(For Utah Teapot).
        const auto proj = makePersp(static_cast<float>(width) / static_cast<float>(height), 1.22, 0.1f, 1000.0f);
        // const auto proj = makeOrtho(-1, 1, -1, 1, -1, 1);
        auto model = flcn::Mat4F::makeRotationY(rot);
        rot += 0.01f;
        model(1, 3) = 0;
        for (auto& vert : vertices)
        {
            // std::cout << vert;
            vert = perVertex(vert, { .model = model, .view = view, .projection = proj });
            // std::cout << vert << '\n';
        }


        [[maybe_unused]] size_t i = 0;
        for (const auto& index : mesh.indices)
        {
            const auto i0 = static_cast<size_t>(index.x());
            const auto i1 = static_cast<size_t>(index.y());
            const auto i2 = static_cast<size_t>(index.z());
            // FIXME: Test code
            const auto faceColor = mesh.colors[i++];
            renderTriangle(vertices[i0], vertices[i1], vertices[i2], faceColor.r(), faceColor.g(), faceColor.b(), 0xff);
            // renderTriangle(vertices[i0], vertices[i1], vertices[i2], 0xff, 0xff, 0xff, 0xff);
            // const auto v0 = vertices[i0].swizzle<flcn::axis::X, flcn::axis::Y>();
            // const auto v1 = vertices[i1].swizzle<flcn::axis::X, flcn::axis::Y>();
            // const auto v2 = vertices[i2].swizzle<flcn::axis::X, flcn::axis::Y>();
            // renderTriangleWireframe(v0, v1, v2);
        }
    }
} // namespace rasterizer
