#pragma once
/**
 * @file ImGui.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 19, 2026
 *
 * @brief $BRIEF$
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

namespace colloid::imgui
{
    void beginFrame()
    {
        const ImGuiIO& io = ImGui::GetIO();
        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void endFrame(SDL_GPUDevice* device, SDL_Window* window)
    {

        ImVec4 clearColor{ 0, 0, 0, 0 };
        // Rendering
        ImGui::Render();
        ImDrawData* draw_data   = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device); // Acquire a GPU command buffer

        SDL_GPUTexture* swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr,
                                              nullptr); // Acquire a swapchain texture

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            // Setup and start a render pass
            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture                = swapchain_texture;
            target_info.clear_color            = SDL_FColor{ clearColor.x, clearColor.y, clearColor.z, clearColor.w };
            target_info.load_op                = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op               = SDL_GPU_STOREOP_STORE;
            target_info.mip_level              = 0;
            target_info.layer_or_depth_plane   = 0;
            target_info.cycle                  = false;
            SDL_GPURenderPass* render_pass     = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }

        // Submit the command buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

} // namespace colloid::imgui
