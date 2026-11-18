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

#include <SDL3/SDL.h>
#include <falcon/Falcon.h>

namespace colloid
{
    class Camera
    {
        static constexpr float MOVE_SENSITIVITY  = 0.001f;
        static constexpr float MOUSE_SENSITIVITY = 0.001f;

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

        inline void update()
        {
            const auto rotation = getRotationMatrix();
            _position += (rotation * flcn::Vec4F(_velocity * MOVE_SENSITIVITY, 0.0f))
                            .swizzle<flcn::axis::X, flcn::axis::Y, flcn::axis::Z>();
        }


        inline void handleSDLEvent(SDL_Event& e)
        {
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.scancode == SDL_SCANCODE_W)
                {
                    _velocity.z() = -1;
                }
                if (e.key.scancode == SDL_SCANCODE_S)
                {
                    _velocity.z() = 1;
                }
                if (e.key.scancode == SDL_SCANCODE_A)
                {
                    _velocity.x() = -1;
                }
                if (e.key.scancode == SDL_SCANCODE_D)
                {
                    _velocity.x() = 1;
                }
            }

            if (e.type == SDL_EVENT_KEY_UP)
            {
                if (e.key.scancode == SDL_SCANCODE_W)
                {
                    _velocity.z() = 0;
                }
                if (e.key.scancode == SDL_SCANCODE_S)
                {
                    _velocity.z() = 0;
                }
                if (e.key.scancode == SDL_SCANCODE_A)
                {
                    _velocity.x() = 0;
                }
                if (e.key.scancode == SDL_SCANCODE_D)
                {
                    _velocity.x() = 0;
                }
            }

            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                _yaw += e.motion.xrel / MOUSE_SENSITIVITY;
                _pitch -= e.motion.yrel / MOUSE_SENSITIVITY;
            }
        }


    private:
        float _yaw = 0, _pitch = 0;
        flcn::Vec3F _velocity, _position;
    };
} // namespace colloid
