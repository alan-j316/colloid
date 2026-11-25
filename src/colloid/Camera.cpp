/**
 * @file Camera.cpp
 * @author Alan Abraham P Kochumon
 * @date Created on: November 24, 2026
 *
 * @brief Implementation of member function declared in Camera.h
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include "Camera.h"
#include <SDL3/SDL.h>


namespace colloid
{
    void Camera::handleSDLEvent(const SDL_Event& e) noexcept
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
            if (e.key.scancode == SDL_SCANCODE_Q)
            {
                _velocity.y() = -1;
            }
            if (e.key.scancode == SDL_SCANCODE_E)
            {
                _velocity.y() = 1;
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
            if (e.key.scancode == SDL_SCANCODE_Q)
            {
                _velocity.y() = 0;
            }
            if (e.key.scancode == SDL_SCANCODE_E)
            {
                _velocity.y() = 0;
            }
        }

        // if (e.type == SDL_EVENT_MOUSE_MOTION)
        // {
        //     _yaw += e.motion.xrel / MOUSE_SENSITIVITY;
        //     _pitch -= e.motion.yrel / MOUSE_SENSITIVITY;
        // }
    }
} // namespace colloid
