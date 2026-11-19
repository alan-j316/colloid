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

#include "ImGui.h"

#include <SDL3/SDL_gpu.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <filesystem>
#include <imgui.h>

namespace rasterizer
{
    Application::Application(const std::string_view appName, const std::string_view version,
                             const std::string_view identifier)
        : _renderer(_width, _height, _colorChannels)
    {
        SDL_SetAppMetadata(appName.data(), version.data(), identifier.data());

        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_Log("There was an error initializing SDL application!");
            return;
        }

        // TODO: Make window resizeable
        _window = SDL_CreateWindow(appName.data(), _width, _height, 0);


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
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch
        io.ConfigDpiScaleFonts = true;

        // Customization
        float mainScale   = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        // style.FontSizeBase = 16.0f;           // Default to 13.0f when unspecified, which allows crisp legacy font
        // for non-4K screens.
        style.FrameRounding = 3.0f;
        style.GrabRounding  = 3.0f;

        // Setup scaling
        style.ScaleAllSizes(mainScale); // Bake a fixed style scale. (until we have a solution for dynamic style
                                        // scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi =
            mainScale; // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically
                       // overrides this for every window depending on the current monitor)


        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU(_window);
        ImGui_ImplSDLGPU3_InitInfo imguiInitInfo = {};
        imguiInitInfo.Device                     = _device;
        imguiInitInfo.ColorTargetFormat          = SDL_GetGPUSwapchainTextureFormat(_device, _window);
        imguiInitInfo.MSAASamples                = SDL_GPU_SAMPLECOUNT_1;      // Only used in multi-viewports mode.
        imguiInitInfo.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
        imguiInitInfo.PresentMode          = SDL_GPU_PRESENTMODE_VSYNC;
        ImGui_ImplSDLGPU3_Init(&imguiInitInfo);

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
        SDL_GPUTransferBuffer* stagingBuffer              = SDL_CreateGPUTransferBuffer(_device, &stagingBufferInfo);
        assert(stagingBuffer != nullptr);

        // Copy the frame buffer to the staging buffer
        void* pTexture = SDL_MapGPUTransferBuffer(_device, stagingBuffer, true);
        memcpy(pTexture, _renderer.frameBuffer, static_cast<size_t>(_width * _height * 4));
        SDL_UnmapGPUTransferBuffer(_device, stagingBuffer);

        // Submit the staging buffer with a copy/transfer pass.
        const SDL_GPUTextureTransferInfo transferInfo{ .transfer_buffer = stagingBuffer, .offset = 0 };

        SDL_GPUTextureRegion textureRegion = { .texture = _renderTexture,
                                               .x       = 0,
                                               .y       = 0,
                                               .w       = static_cast<uint32_t>(_width),
                                               .h       = static_cast<uint32_t>(_height),
                                               .d       = 1 };

        // Upload
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
        SDL_UploadToGPUTexture(copyPass, &transferInfo, &textureRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(cmd);
        // TODO: TODO REMOVE THIS AND MAKE THE STAGING BUFFER PERMANENT
        SDL_ReleaseGPUTransferBuffer(_device, stagingBuffer);
    }


    void Application::renderRasterizedOutput() const noexcept
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // NOLINTBEGIN
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBackground;
        // NOLINTEND

        ImGui::Begin("Rasterization Output", nullptr, windowFlags);
        ImGui::Image(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(_renderTexture)),
                     ImVec2(static_cast<float>(_width), static_cast<float>(_height)));
        ImGui::PopStyleVar(2);
        ImGui::End();
    }


    void Application::renderWidgets() noexcept
    {
        bool showDemoWindow = false;

        auto& io = ImGui::GetIO();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its
        // code to learn more about Dear ImGui!).
        if (showDemoWindow)
        {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f     = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");        // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &showDemoWindow); // Edit bools storing our window open/close state

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f

            if (ImGui::Button(
                    "Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
    }


    void Application::run() noexcept
    {
        // auto modelPath = std::filesystem::path("models") / "utah_teapot.obj";
        auto modelPath = std::filesystem::path("models") / "suzanne.obj";
        // auto modelPath = std::filesystem::path("models") / "Cube.obj";
        ObjectParser parser;
        auto mesh = parser.parse(modelPath.string());

        _isRunning = true;


        SDL_Surface* renderingSurface = SDL_GetWindowSurface(_window);

        _renderer.clearScreen();
        _renderer.render(mesh, 0.0f);

        float rotationAngle = 0.0f;
        while (_isRunning)
        {
            blitTexture();

            colloid::imgui::beginFrame();
            renderRasterizedOutput();
            renderWidgets();
            colloid::imgui::endFrame(_device, _window);
            //
            // SDL_BlitSurface(imageSurface, nullptr, renderingSurface, nullptr);
            // SDL_UpdateWindowSurface(_window);
            pollEvent();

            rotationAngle += 0.1f;

            /// +---------------------------------------------------------------------------------------+
            /// |                                 DEAR IMGUI                                            |
            /// +---------------------------------------------------------------------------------------+
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
} // namespace rasterizer
