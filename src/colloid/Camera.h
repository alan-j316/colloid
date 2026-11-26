#pragma once
/**
 * @file Camera.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 18, 2026
 *
 * @brief Simple camera.
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */



#include <falcon/Falcon.h>

/// Forward declaration
union SDL_Event;

namespace colloid
{
    class Camera
    {
    public:
        static constexpr float MOVE_SENSITIVITY  = 10.0f;
        static constexpr float MOUSE_SENSITIVITY = 0.0f; // 0.00001f; TODO: Fix swinging

        [[nodiscard]] flcn::Mat4F getViewMatrix() const noexcept
        {
            flcn::Mat4F translationMat = flcn::Mat4F::identity();
            translationMat(0, 3)       = _position.x();
            translationMat(1, 3)       = _position.y();
            translationMat(2, 3)       = _position.z();

            const auto rotationMat = getRotationMatrix();

            return (translationMat * rotationMat).inverse();
        }


        [[nodiscard]] inline flcn::Mat4F getRotationMatrix() const
        {
            const flcn::Mat4F rotPitch = flcn::Mat4F::makeRotationX(_pitch);
            const flcn::Mat4F rotYaw   = flcn::Mat4F::makeRotationY(-_yaw);

            return rotPitch * rotYaw;
        }

        inline void update(const float deltaTime)
        {
            const auto rotation = getRotationMatrix();
            _position += (rotation * flcn::Vec4F(_velocity * MOVE_SENSITIVITY * deltaTime, 0.0f))
                             .swizzle<flcn::axis::X, flcn::axis::Y, flcn::axis::Z>();
        }


        void handleSDLEvent(const SDL_Event& e) noexcept;



    private:
        float _yaw = 0, _pitch = 0;
        flcn::Vec3F _velocity, _position{ 0, 0, 4 };
    };
} // namespace colloid
