#pragma once
/**
 * @file Gui.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 19, 2026
 *
 * @brief $BRIEF$
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

namespace colloid::imgui
{

    inline void init(SDL_GPUDevice* device, SDL_Window* window) noexcept
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // NOLINTBEGIN
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch
        // NOLINTEND
        io.ConfigDpiScaleFonts = true;


        // Customization
        const float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        ImGuiStyle& style     = ImGui::GetStyle();
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        // style.FontSizeBase = 16.0f;           // Default to 13.0f when unspecified, which allows crisp legacy font
        // for non-4K screens.
        style.FrameRounding = 3.0f;
        style.GrabRounding  = 3.0f;


        // Setup scaling
        // Bake a fixed style scale. (until we have a solution for dynamic style
        // scaling, changing this requires resetting Style + calling this again)
        style.ScaleAllSizes(mainScale);
        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically
        // overrides this for every window depending on the current monitor)
        style.FontScaleDpi = mainScale;


        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_InitInfo imguiInitInfo = { .Device = device,
                                                     .ColorTargetFormat =
                                                         SDL_GetGPUSwapchainTextureFormat(device, window),
                                                     .MSAASamples          = SDL_GPU_SAMPLECOUNT_1,
                                                     .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                                     .PresentMode          = SDL_GPU_PRESENTMODE_VSYNC };
        ImGui_ImplSDLGPU3_Init(&imguiInitInfo);
    }

    inline void beginFrame() noexcept
    {
        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }


    inline void endFrame(SDL_GPUDevice* device, SDL_Window* window) noexcept
    {

        ImVec4 clearColor{ 0, 0, 0, 0 };
        // Rendering
        ImGui::Render();
        ImDrawData* drawData   = ImGui::GetDrawData();
        const bool isMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(device); // Acquire a GPU command buffer

        SDL_GPUTexture* swapchainTexture;
        SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, window, &swapchainTexture, nullptr,
                                              nullptr); // Acquire a swapchain texture

        if (swapchainTexture != nullptr && !isMinimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmdBuffer);

            // Setup and start a render pass
            SDL_GPUColorTargetInfo targetInfo = {
                .texture              = swapchainTexture,
                .mip_level            = 0,
                .layer_or_depth_plane = 0,
                .clear_color = SDL_FColor{ .r = clearColor.x, .g = clearColor.y, .b = clearColor.z, .a = clearColor.w },
                .load_op     = SDL_GPU_LOADOP_CLEAR,
                .store_op    = SDL_GPU_STOREOP_STORE,
                .cycle       = false
            };
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &targetInfo, 1, nullptr);

            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmdBuffer, renderPass);
            SDL_EndGPURenderPass(renderPass);
        }

        // Submit the command buffer
        SDL_SubmitGPUCommandBuffer(cmdBuffer);
    }

} // namespace colloid::imgui
