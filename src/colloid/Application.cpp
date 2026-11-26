/**
 * @file Application.cpp
 * @author Alan Abraham P Kochumon
 * @date Created on: November 09, 2026
 *
 * @brief Implemenat
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */


#include "colloid/Application.h"

#include "Gui.h" // TODO: Rename to an apt name

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <filesystem>
#include <imgui.h>

namespace colloid
{
    Application::Application(std::string appName, std::string version, std::string identifier)
        : _title(std::move(appName)),
          _version(std::move(version)),
          _identifier(std::move(identifier)),
          _renderer(_width, _height, _colorChannels)
    {
        SDL_SetAppMetadata(_title.c_str(), _version.c_str(), _identifier.c_str());

        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_Log("There was an error initializing SDL application!");
            return;
        }

        // TODO: Make window resizeable
        _window = SDL_CreateWindow(_title.c_str(), _width, _height, 0);


        /// +---------------------------------------------------------------------------------------+
        /// |                            SDL GPU(USED BY IMGUI)                                     |
        /// +---------------------------------------------------------------------------------------+

        SDL_GPUShaderFormat shaderFormat =
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;

        _device = SDL_CreateGPUDevice(shaderFormat, true, nullptr);
        if (_device == nullptr)
        {
            SDL_Log("Failed to create GPU Device: %s", SDL_GetError());
        }

        if (!SDL_ClaimWindowForGPUDevice(_device, _window))
        {
            SDL_Log("Failed to claim window: %s", SDL_GetError());
            SDL_DestroyGPUDevice(_device);
        }


        /// +---------------------------------------------------------------------------------------+
        /// |                                   IMGUI                                               |
        /// +---------------------------------------------------------------------------------------+
        colloid::imgui::init(_device, _window);

        initRenderTexture();
    }


    void Application::initRenderTexture() noexcept
    {
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width                    = _width;
        textureInfo.height                   = _height;
        textureInfo.layer_count_or_depth     = 1;
        textureInfo.num_levels               = 1;
        textureInfo.sample_count             = SDL_GPU_SAMPLECOUNT_1;

        _renderTexture = SDL_CreateGPUTexture(_device, &textureInfo);
    }


    void Application::pollEvent() noexcept
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            _mainCamera.handleSDLEvent(event);
            ImGui_ImplSDL3_ProcessEvent(&event); // Forward your event to backend
            // (You should discard mouse/keyboard messages in your game/engine when
            // io.WantCaptureMouse/io.WantCaptureKeyboard are set.)
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    _isRunning = false;
                    break;
                default:
                    break;
                    // SDL_Log("[EVENT]: Unhandled"); // TODO: Remove
            }
        }
    }


    void Application::blitTexture() const noexcept
    {
        // Create staging buffer
        // FIXME: A real engine would likely keep one around, see what the SDL_GPU backend is doing.
        SDL_GPUTransferBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        stagingBufferInfo.size                            = _width * _height * 4;
        SDL_GPUTransferBuffer* pStagingBuffer             = SDL_CreateGPUTransferBuffer(_device, &stagingBufferInfo);
        assert(pStagingBuffer != nullptr);

        // Copy the frame buffer to the staging buffer
        void* pTexture = SDL_MapGPUTransferBuffer(_device, pStagingBuffer, true);
        memcpy(pTexture, _renderer.frameBuffer, static_cast<size_t>(_width * _height * 4));
        SDL_UnmapGPUTransferBuffer(_device, pStagingBuffer);

        // Submit the staging buffer with a copy/transfer pass.
        const SDL_GPUTextureTransferInfo transferInfo{ .transfer_buffer = pStagingBuffer, .offset = 0 };

        const SDL_GPUTextureRegion textureRegion = { .texture = _renderTexture,
                                                     .x       = 0,
                                                     .y       = 0,
                                                     .w       = static_cast<uint32_t>(_width),
                                                     .h       = static_cast<uint32_t>(_height),
                                                     .d       = 1 };

        // Upload
        SDL_GPUCommandBuffer* pCmdBuffer = SDL_AcquireGPUCommandBuffer(_device);
        SDL_GPUCopyPass* pCopyPass       = SDL_BeginGPUCopyPass(pCmdBuffer);
        SDL_UploadToGPUTexture(pCopyPass, &transferInfo, &textureRegion, false);
        SDL_EndGPUCopyPass(pCopyPass);
        SDL_SubmitGPUCommandBuffer(pCmdBuffer);
        // TODO: TODO REMOVE THIS AND MAKE THE STAGING BUFFER PERMANENT
        SDL_ReleaseGPUTransferBuffer(_device, pStagingBuffer);
    }


    void Application::renderRasterizedOutput() const noexcept
    {
        const ImGuiViewport* pViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(pViewport->WorkPos);
        ImGui::SetNextWindowSize(pViewport->WorkSize);
        ImGui::SetNextWindowViewport(pViewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        // NOLINTBEGIN
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBackground;
        // NOLINTEND

        ImGui::Begin("Rasterization Output", nullptr, windowFlags);
        ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(_renderTexture)),
                     ImVec2(static_cast<float>(_width), static_cast<float>(_height)));
        ImGui::PopStyleVar(3);
        ImGui::End();
    }


    void Application::renderWidgets() noexcept
    {
        bool showDemoWindow = false;
        if (showDemoWindow)
        {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        const auto& io = ImGui::GetIO();
        ImGui::Begin("Rendering Timings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", _stats.frameTime, 1000.0f / _stats.frameTime);
        ImGui::Text("Update time: %.3f ms/frame ", _stats.updateTime);
        ImGui::Text("Draw time: %.3f ms/frame", _stats.drawTime);
        ImGui::End();
    }


    void Application::run() noexcept
    {
        // TODO: Move from run
        // auto modelPath = std::filesystem::path("models") / "utah_teapot.obj";
        const auto modelPath = std::filesystem::path("models") / "suzanne.obj";
        // auto modelPath = std::filesystem::path("models") / "Cube.obj";
        ObjectParser parser;
        const auto mesh     = parser.parse(modelPath.string());
        _isRunning          = true;
        float rotationAngle = 0.0f;

        auto lastTime = std::chrono::steady_clock::now();
        while (_isRunning)
        {
            auto startTime = std::chrono::steady_clock::now(); // Used for engine stats
            pollEvent();

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed     = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
            float deltaTime  = static_cast<float>(elapsed.count()) * static_cast<float>(1e-3);
            _mainCamera.update(deltaTime * 0.001f);
            _renderer.updateView(_mainCamera.getViewMatrix());

            // Stats Update Time
            currentTime = std::chrono::steady_clock::now();
            elapsed     = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
            deltaTime  = static_cast<float>(elapsed.count()) * static_cast<float>(1e-3);
            _stats.updateTime = deltaTime;
            lastTime = currentTime;

            auto renderStartTime = std::chrono::steady_clock::now();


            _renderer.clearScreen();
            _renderer.render(mesh, 0.0f);
            blitTexture();
            colloid::imgui::beginFrame();
            renderRasterizedOutput();
            renderWidgets();
            colloid::imgui::endFrame(_device, _window);

            currentTime     = std::chrono::steady_clock::now();
            elapsed         = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - renderStartTime);
            deltaTime       = static_cast<float>(elapsed.count()) * static_cast<float>(1e-3);
            _stats.drawTime = deltaTime;


            elapsed          = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
            deltaTime        = static_cast<float>(elapsed.count()) * static_cast<float>(1e-3);
            _stats.frameTime = deltaTime;
            // rotationAngle += 0.01f;
        }


        /// +---------------------------------------------------------------------------------------+
        /// |                       DESTROY INITIALIZED STATES                                      |
        /// +---------------------------------------------------------------------------------------+
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyGPUDevice(_device);
        SDL_DestroyWindow(_window);
        SDL_Quit();
    }
} // namespace colloid
