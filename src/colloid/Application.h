#pragma once
/**
 * @file Application.h
 * @author Alan Abraham P Kochumon
 * @date Created on: June 13, 2026
 *
 * @brief Manages the entire application core and running loop.
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */


#include "colloid/Renderer.h"
#include "parsers/ObjectParser.h"

/// Forward declarations
/// NOLINTBEGIN
struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUTexture;
/// NOLINTEND

namespace colloid
{
    class Application
    {
    public:
        Application(std::string appName, std::string version, std::string identifier);
        void run() noexcept;

    private:
        /// Initializes the texture required for rendering.
        void initRenderTexture() noexcept;
        void pollEvent() noexcept;
        /// Copies the frame buffer to the render texture.
        void blitTexture() const noexcept;
        void renderRasterizedOutput() const noexcept;
        void renderWidgets() noexcept;

    private:
        SDL_Window* _window{ nullptr };
        SDL_GPUDevice* _device;
        SDL_GPUTexture* _renderTexture; /// < Texture for rendering the rasterized output to.
        std::string _title, _version, _identifier;
        bool _isRunning{ false };
        int _width{ 1280 }, _height{ 720 }, _colorChannels{ 4 };
        Renderer _renderer;
    };

} // namespace colloid
