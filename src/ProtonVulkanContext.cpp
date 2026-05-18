#include "ProtonVulkanContext.hpp"

#include "ProtonLog.hpp"
#include "ProtonDraw2D.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <set>
#include <vector>
#include <cstring>
#include <cstddef>
#include <fstream>
#include <cmath>
#include <utility>
#include <ranges>

namespace {
    constexpr std::array<const char *, 1> REQUIRED_DEVICE_EXTENSIONS =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    constexpr float PI = 3.14159265358979323846f;

    std::vector<char> ReadBinaryFile(const std::string &path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file) {
            Proton::Log::Error(std::format("Failed to open binary file: {}", path));
            return {};
        }

        const std::streamsize fileSize = file.tellg();
        std::vector<char> buffer(static_cast<std::size_t>(fileSize));

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return buffer;
    }

    VkShaderModule CreateShaderModuleFromBytes(
        VkDevice device,
        const std::vector<char> &code
    ) {
        if (code.empty()) {
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        const VkResult result = vkCreateShaderModule(
            device,
            &createInfo,
            nullptr,
            &shaderModule
        );

        if (result != VK_SUCCESS) {
            Proton::Log::Error(std::format(
                "Failed to create Vulkan shader module. VkResult={}",
                static_cast<int>(result)
            ));

            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }
}

namespace Proton {
    struct Rect2DVertex {
        float X;
        float Y;

        float R;
        float G;
        float B;
        float A;
    };

    struct Sprite2DVertex {
        float X;
        float Y;

        float U;
        float V;

        float R;
        float G;
        float B;
        float A;
    };

    VulkanContext::~VulkanContext() {
        Shutdown();
    }

    bool VulkanContext::Init(GLFWwindow *window) {
        Log::Info("Initializing Vulkan context...");
        m_Window = window;

        if (!CreateInstance()) return false;
        if (!CreateSurface(window)) return false;
        if (!PickPhysicalDevice()) return false;
        if (!CreateLogicalDevice()) return false;
        if (!CreateSwapchain()) return false;
        if (!CreateImageViews()) return false;
        if (!CreateRenderPass()) return false;
        if (!CreateFramebuffers()) return false;
        if (!CreateCommandPool()) return false;
        if (!CreateCommandBuffers()) return false;
        if (!CreateSyncObjects()) return false;
        if (!CreateDescriptorSetLayout()) return false;
        if (!CreateSpriteDescriptorSetLayout()) return false;
        if (!CreateGraphicsPipeline()) return false;
        if (!CreateRect2DPipeline()) return false;
        if (!CreateSprite2DPipeline()) return false;
        if (!CreateDescriptorPool()) return false;

        Log::Info("Vulkan context initialized.");
        return true;
    }

    void VulkanContext::Shutdown() {
        /*
            ----------------------------------------------------------------------
            DESTROY FRAMEBUFFERS
            ----------------------------------------------------------------------

            Framebuffers reference:
            - Render pass
            - Image views

            Therefore they MUST be destroyed BEFORE:
            - render pass destruction
            - image view destruction
        */
        if (m_Device != VK_NULL_HANDLE &&
            m_SwapchainFramebuffers != nullptr) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                if (m_SwapchainFramebuffers[i] != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(
                        m_Device,
                        m_SwapchainFramebuffers[i],
                        nullptr
                    );
                }
            }

            m_SwapchainFramebuffers.reset();
        }

        /*
            ----------------------------------------------------------------------
            DESTROY IMAGE VIEWS
            ----------------------------------------------------------------------

            Image views wrap swapchain images so Vulkan can interpret them
            as renderable color attachments.
        */
        if (m_Device != VK_NULL_HANDLE &&
            m_SwapchainImageViews != nullptr) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                if (m_SwapchainImageViews[i] != VK_NULL_HANDLE) {
                    vkDestroyImageView(
                        m_Device,
                        m_SwapchainImageViews[i],
                        nullptr
                    );
                }
            }

            m_SwapchainImageViews.reset();
        }

        // BEFORE DESTROYING THE RENDER PASS
        // ALWAYS CLEAN UP THE SWAPCHAIN
        // RESOURCES AND DO EXTRA
        // SWAPCHAIN RELATED CLEANUP
        CleanupSwapchainResources();

        // ALONG WITH 2D RESOURCES
        DestroyDraw2DResources();

        if (m_Device != VK_NULL_HANDLE && m_Rect2DPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_Device, m_Rect2DPipeline, nullptr);
            m_Rect2DPipeline = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_Rect2DPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device, m_Rect2DPipelineLayout, nullptr);
            m_Rect2DPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_GraphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
            m_GraphicsPipeline = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
            m_TextureDescriptorSet = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_TextureDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_TextureDescriptorSetLayout, nullptr);
            m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY RENDER PASS
            ----------------------------------------------------------------------

            Render pass describes:
            - attachment formats
            - load/store operations
            - presentation layouts
        */
        if (m_Device != VK_NULL_HANDLE &&
            m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(
                m_Device,
                m_RenderPass,
                nullptr
            );

            m_RenderPass = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY SWAPCHAIN
            ----------------------------------------------------------------------

            Swapchain owns the images presented to the window surface.
        */
        if (m_Device != VK_NULL_HANDLE &&
            m_Swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(
                m_Device,
                m_Swapchain,
                nullptr
            );

            m_Swapchain = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            RELEASE CPU-SIDE SWAPCHAIN STORAGE
            ----------------------------------------------------------------------

            These arrays only store Vulkan handles on the CPU side.
        */
        m_SwapchainImages.reset();
        m_SwapchainImageCount = 0;

        /*
            ----------------------------------------------------------------------
            DESTROY Command buffer recording
            ----------------------------------------------------------------------
         */

        if (m_Device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_Device);
        }

        // DESTROY Texture Resources
        DestroyTextureResources();

        /*
            ----------------------------------------------------------------------
            DESTROY Sync Objects
            ----------------------------------------------------------------------
         */

        if (m_Device != VK_NULL_HANDLE) {
            if (m_InFlightFence != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, m_InFlightFence, nullptr);
                m_InFlightFence = VK_NULL_HANDLE;
            }

            if (m_RenderFinishedSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
                m_RenderFinishedSemaphore = VK_NULL_HANDLE;
            }

            if (m_ImageAvailableSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
                m_ImageAvailableSemaphore = VK_NULL_HANDLE;
            }
        }

        /*
            ----------------------------------------------------------------------
            DESTROY Command Pools/Buffers
            ----------------------------------------------------------------------
         */

        if (m_Device != VK_NULL_HANDLE &&
            m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(
                m_Device,
                m_CommandPool,
                nullptr
            );

            m_CommandBuffers.reset();
            m_CommandPool = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY Sprite Resources
            ----------------------------------------------------------------------
         */
        DestroySprite2DResources();

        if (m_Device != VK_NULL_HANDLE && m_Sprite2DPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_Device, m_Sprite2DPipeline, nullptr);
            m_Sprite2DPipeline = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_Sprite2DPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_Device, m_Sprite2DPipelineLayout, nullptr);
            m_Sprite2DPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE && m_SpriteDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_Device, m_SpriteDescriptorSetLayout, nullptr);
            m_SpriteDescriptorSetLayout = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY LOGICAL DEVICE
            ----------------------------------------------------------------------

            The logical device represents Proton G's connection to the GPU.
        */
        if (m_Device != VK_NULL_HANDLE) {
            vkDestroyDevice(
                m_Device,
                nullptr
            );

            m_Device = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY WINDOW SURFACE
            ----------------------------------------------------------------------

            The surface bridges GLFW/windowing with Vulkan presentation.
        */
        if (m_Instance != VK_NULL_HANDLE &&
            m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(
                m_Instance,
                m_Surface,
                nullptr
            );

            m_Surface = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            DESTROY VULKAN INSTANCE
            ----------------------------------------------------------------------

            The Vulkan instance is the root Vulkan object.
        */
        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(
                m_Instance,
                nullptr
            );

            m_Instance = VK_NULL_HANDLE;
        }

        /*
            ----------------------------------------------------------------------
            CLEAR REMAINING HANDLES
            ----------------------------------------------------------------------
        */
        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
    }

    bool VulkanContext::CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Proton G";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "Proton G";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        uint32_t extensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

        if (glfwExtensions == nullptr || extensionCount == 0) {
            Log::Error("GLFW did not provide Vulkan instance extensions.");
            return false;
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);

        if (result != VK_SUCCESS) {
            Log::Error(std::format("Failed to create Vulkan instance. VkResult={}", static_cast<int>(result)));
            return false;
        }

        Log::Info("Vulkan instance created.");
        return true;
    }

    bool VulkanContext::CreateSurface(GLFWwindow *window) {
        const VkResult result = glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface);

        if (result != VK_SUCCESS) {
            Log::Error(std::format("Failed to create Vulkan window surface. VkResult={}", static_cast<int>(result)));
            return false;
        }

        Log::Info("Vulkan window surface created.");
        return true;
    }

    bool VulkanContext::CreateFramebuffers() {
        m_SwapchainFramebuffers =
                std::make_unique<VkFramebuffer[]>(m_SwapchainImageCount);

        for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
            m_SwapchainFramebuffers[i] = VK_NULL_HANDLE;

            VkImageView attachments[] =
            {
                m_SwapchainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType =
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

            framebufferInfo.renderPass = m_RenderPass;

            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;

            framebufferInfo.width = m_SwapchainExtent.width;
            framebufferInfo.height = m_SwapchainExtent.height;
            framebufferInfo.layers = 1;

            const VkResult result = vkCreateFramebuffer(
                m_Device,
                &framebufferInfo,
                nullptr,
                &m_SwapchainFramebuffers[i]
            );

            if (result != VK_SUCCESS) {
                Log::Error(std::format(
                    "Failed to create framebuffer {}. VkResult={}",
                    i,
                    static_cast<int>(result)
                ));

                return false;
            }
        }

        Log::Info(std::format(
            "Vulkan framebuffers created. Count: {}",
            m_SwapchainImageCount
        ));

        return true;
    }

    bool VulkanContext::CreateCommandPool() {
        const QueueFamilyIndices queueFamilyIndices =
                FindQueueFamilies(m_PhysicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        poolInfo.flags =
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        poolInfo.queueFamilyIndex =
                queueFamilyIndices.GraphicsFamily.value();

        const VkResult result = vkCreateCommandPool(
            m_Device,
            &poolInfo,
            nullptr,
            &m_CommandPool
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan command pool. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan command pool created.");
        return true;
    }

    bool VulkanContext::CreateCommandBuffers() {
        m_CommandBuffers =
                std::make_unique<VkCommandBuffer[]>(m_SwapchainImageCount);

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = m_SwapchainImageCount;

        const VkResult result = vkAllocateCommandBuffers(
            m_Device,
            &allocateInfo,
            m_CommandBuffers.get()
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to allocate Vulkan command buffers. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info(std::format(
            "Vulkan command buffers allocated. Count: {}",
            m_SwapchainImageCount
        ));

        return true;
    }

    bool VulkanContext::RecordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex
    ) {
        if (!UpdateDraw2DVertexBuffer()) {
            return false;
        }

        if (!EnsureSpriteTexturesForFrame()) {
            return false;
        }

        const std::vector<SpriteDrawCommand> *spriteCommands = nullptr;

        if (m_SpriteManager != nullptr) {
            spriteCommands = &m_SpriteManager->GetDrawCommands();

            if (!UpdateSprite2DVertexBuffer(*spriteCommands)) {
                return false;
            }
        }

        BuildRenderQueue();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        const VkResult beginResult = vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo
        );

        if (beginResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to begin command buffer. VkResult={}",
                static_cast<int>(beginResult)
            ));

            return false;
        }

        VkClearValue clearColor{};
        clearColor.color = m_ClearColor;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_SwapchainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_SwapchainExtent;

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE
        );

        /*
            Set viewport/scissor once for all pipelines.
        */
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_SwapchainExtent.width);
        viewport.height = static_cast<float>(m_SwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_SwapchainExtent;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        /*
            Keep this image block.
        */
        if (m_GraphicsPipeline != VK_NULL_HANDLE &&
            m_TextureDescriptorSet != VK_NULL_HANDLE &&
            m_TextureImageView != VK_NULL_HANDLE) {
            struct ImageScalePushConstants {
                float ImageWidth;
                float ImageHeight;
                float ViewportWidth;
                float ViewportHeight;
                int ScaleMode;
            };

            ImageScalePushConstants scaleData{};
            scaleData.ImageWidth = static_cast<float>(m_TextureWidth);
            scaleData.ImageHeight = static_cast<float>(m_TextureHeight);
            scaleData.ViewportWidth = static_cast<float>(m_SwapchainExtent.width);
            scaleData.ViewportHeight = static_cast<float>(m_SwapchainExtent.height);
            scaleData.ScaleMode = m_ImageScaleMode;

            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_GraphicsPipeline
            );

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_PipelineLayout,
                0,
                1,
                &m_TextureDescriptorSet,
                0,
                nullptr
            );

            vkCmdPushConstants(
                commandBuffer,
                m_PipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(ImageScalePushConstants),
                &scaleData
            );

            vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        }

        for (const RenderQueueEntry &entry: m_RenderQueue) {
            if (entry.Type == RenderQueueEntryType::Draw2D) {
                if (entry.BatchIndex >= m_Draw2DLayerBatches.size()) {
                    continue;
                }

                const Draw2DLayerBatch &batch =
                        m_Draw2DLayerBatches[entry.BatchIndex];

                if (m_Rect2DPipeline == VK_NULL_HANDLE ||
                    m_Rect2DVertexBuffer == VK_NULL_HANDLE ||
                    batch.VertexCount == 0) {
                    continue;
                }

                struct Rect2DPushConstants {
                    float ViewportWidth;
                    float ViewportHeight;
                };

                Rect2DPushConstants viewportData{};
                viewportData.ViewportWidth =
                        static_cast<float>(m_SwapchainExtent.width);
                viewportData.ViewportHeight =
                        static_cast<float>(m_SwapchainExtent.height);

                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_Rect2DPipeline
                );

                VkDeviceSize offsets[] = {0};

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &m_Rect2DVertexBuffer,
                    offsets
                );

                vkCmdPushConstants(
                    commandBuffer,
                    m_Rect2DPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(Rect2DPushConstants),
                    &viewportData
                );

                vkCmdDraw(
                    commandBuffer,
                    batch.VertexCount,
                    1,
                    batch.FirstVertex,
                    0
                );
            } else if (entry.Type == RenderQueueEntryType::Sprite2D) {
                if (entry.BatchIndex >= m_Sprite2DBatches.size()) {
                    continue;
                }

                const Sprite2DBatch &batch =
                        m_Sprite2DBatches[entry.BatchIndex];

                if (m_Sprite2DPipeline == VK_NULL_HANDLE ||
                    m_Sprite2DVertexBuffer == VK_NULL_HANDLE ||
                    batch.VertexCount == 0) {
                    continue;
                }

                const auto textureIterator =
                        m_SpriteTextures.find(batch.Handle);

                if (textureIterator == m_SpriteTextures.end()) {
                    continue;
                }

                const SpriteGpuTexture &texture = textureIterator->second;

                if (texture.DescriptorSet == VK_NULL_HANDLE) {
                    continue;
                }

                struct Sprite2DPushConstants {
                    float ViewportWidth;
                    float ViewportHeight;
                };

                Sprite2DPushConstants viewportData{};
                viewportData.ViewportWidth =
                        static_cast<float>(m_SwapchainExtent.width);
                viewportData.ViewportHeight =
                        static_cast<float>(m_SwapchainExtent.height);

                vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_Sprite2DPipeline
                );

                VkDeviceSize offsets[] = {0};

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    1,
                    &m_Sprite2DVertexBuffer,
                    offsets
                );

                vkCmdPushConstants(
                    commandBuffer,
                    m_Sprite2DPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(Sprite2DPushConstants),
                    &viewportData
                );

                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_Sprite2DPipelineLayout,
                    0,
                    1,
                    &texture.DescriptorSet,
                    0,
                    nullptr
                );

                vkCmdDraw(
                    commandBuffer,
                    batch.VertexCount,
                    1,
                    batch.FirstVertex,
                    0
                );
            }
        }

        vkCmdEndRenderPass(commandBuffer);

        const VkResult endResult = vkEndCommandBuffer(commandBuffer);

        if (endResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to end command buffer. VkResult={}",
                static_cast<int>(endResult)
            ));

            return false;
        }

        return true;
    }

    void VulkanContext::NotifyFramebufferResized() {
        m_FramebufferResized = true;
    }

    void VulkanContext::CleanupSwapchainResources() {
        if (m_Device == VK_NULL_HANDLE) {
            return;
        }

        if (m_SwapchainFramebuffers != nullptr) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                if (m_SwapchainFramebuffers[i] != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(m_Device, m_SwapchainFramebuffers[i], nullptr);
                }
            }

            m_SwapchainFramebuffers.reset();
        }

        if (m_SwapchainImageViews != nullptr) {
            for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
                if (m_SwapchainImageViews[i] != VK_NULL_HANDLE) {
                    vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
                }
            }

            m_SwapchainImageViews.reset();
        }

        if (m_Swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        m_SwapchainImages.reset();
        m_CommandBuffers.reset();
        m_SwapchainImageCount = 0;
    }

    bool VulkanContext::RecreateSwapchain() {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(m_Window, &width, &height);

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_Window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_Device);

        CleanupSwapchainResources();

        if (!CreateSwapchain()) {
            return false;
        }

        if (!CreateImageViews()) {
            return false;
        }

        if (!CreateFramebuffers()) {
            return false;
        }

        if (!CreateCommandBuffers()) {
            return false;
        }

        m_FramebufferResized = false;

        Log::Info(std::format(
            "Vulkan swapchain recreated. New extent: {}x{}",
            m_SwapchainExtent.width,
            m_SwapchainExtent.height
        ));

        return true;
    }

    bool VulkanContext::CreateSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        /*
            The fence starts signaled because the first frame has no previous GPU
            work to wait for. Without this flag, the first frame can wait forever.
        */
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        const VkResult imageSemaphoreResult = vkCreateSemaphore(
            m_Device,
            &semaphoreInfo,
            nullptr,
            &m_ImageAvailableSemaphore
        );

        const VkResult renderSemaphoreResult = vkCreateSemaphore(
            m_Device,
            &semaphoreInfo,
            nullptr,
            &m_RenderFinishedSemaphore
        );

        const VkResult fenceResult = vkCreateFence(
            m_Device,
            &fenceInfo,
            nullptr,
            &m_InFlightFence
        );

        if (imageSemaphoreResult != VK_SUCCESS ||
            renderSemaphoreResult != VK_SUCCESS ||
            fenceResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan sync objects. ImageSemaphore={}, RenderSemaphore={}, Fence={}",
                static_cast<int>(imageSemaphoreResult),
                static_cast<int>(renderSemaphoreResult),
                static_cast<int>(fenceResult)
            ));

            return false;
        }

        Log::Info("Vulkan sync objects created.");
        return true;
    }

    void VulkanContext::DrawFrame() {
        vkWaitForFences(
            m_Device,
            1,
            &m_InFlightFence,
            VK_TRUE,
            UINT64_MAX
        );

        vkResetFences(
            m_Device,
            1,
            &m_InFlightFence
        );

        uint32_t imageIndex = 0;

        const VkResult acquireResult = vkAcquireNextImageKHR(
            m_Device,
            m_Swapchain,
            UINT64_MAX,
            m_ImageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return;
        }

        if (acquireResult != VK_SUCCESS &&
            acquireResult != VK_SUBOPTIMAL_KHR) {
            Log::Error(std::format(
                "Failed to acquire swapchain image. VkResult={}",
                static_cast<int>(acquireResult)
            ));

            return;
        }

        vkResetCommandBuffer(
            m_CommandBuffers[imageIndex],
            0
        );

        if (!RecordCommandBuffer(m_CommandBuffers[imageIndex], imageIndex)) {
            return;
        }

        VkSemaphore waitSemaphores[] =
        {
            m_ImageAvailableSemaphore
        };

        VkPipelineStageFlags waitStages[] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        VkSemaphore signalSemaphores[] =
        {
            m_RenderFinishedSemaphore
        };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        const VkResult submitResult = vkQueueSubmit(
            m_GraphicsQueue,
            1,
            &submitInfo,
            m_InFlightFence
        );

        if (submitResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to submit draw command buffer. VkResult={}",
                static_cast<int>(submitResult)
            ));

            return;
        }

        VkSwapchainKHR swapchains[] =
        {
            m_Swapchain
        };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        const VkResult presentResult = vkQueuePresentKHR(
            m_PresentQueue,
            &presentInfo
        );

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
            presentResult == VK_SUBOPTIMAL_KHR ||
            m_FramebufferResized) {
            RecreateSwapchain();
            return;
        }

        if (presentResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to present swapchain image. VkResult={}",
                static_cast<int>(presentResult)
            ));
        }

        if (presentResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to present swapchain image. VkResult={}",
                static_cast<int>(presentResult)
            ));
        }
    }

    bool VulkanContext::PickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            Log::Error("No Vulkan-capable GPU found.");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (VkPhysicalDevice device: devices) {
            const QueueFamilyIndices indices = FindQueueFamilies(device);

            if (!indices.IsComplete()) {
                continue;
            }

            const SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);

            if (swapchainSupport.Formats.empty() || swapchainSupport.PresentModes.empty()) {
                continue;
            }

            m_PhysicalDevice = device;

            Log::Info(std::format("Swapchain formats available: {}", swapchainSupport.Formats.size()));
            Log::Info(std::format("Present modes available: {}", swapchainSupport.PresentModes.size()));
            break;
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE) {
            Log::Error("No suitable Vulkan GPU found.");
            return false;
        }

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

        Log::Info(std::format("Found {} Vulkan GPU(s).", deviceCount));
        Log::Info(std::format("Using GPU: {}", properties.deviceName));

        return true;
    }

    bool VulkanContext::CreateLogicalDevice() {
        const QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        if (!indices.IsComplete()) {
            Log::Error("Cannot create logical device because queue families are incomplete.");
            return false;
        }

        const std::set<uint32_t> uniqueQueueFamilies =
        {
            indices.GraphicsFamily.value(),
            indices.PresentFamily.value()
        };

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size());
        createInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data();

        const VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);

        if (result != VK_SUCCESS) {
            Log::Error(std::format("Failed to create Vulkan logical device. VkResult={}", static_cast<int>(result)));
            return false;
        }

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);

        Log::Info(std::format("Graphics queue family: {}", indices.GraphicsFamily.value()));
        Log::Info(std::format("Present queue family: {}", indices.PresentFamily.value()));
        Log::Info("Vulkan logical device created.");

        return true;
    }

    bool VulkanContext::CreateSwapchain() {
        const SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(m_PhysicalDevice);
        const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.Formats);
        const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.PresentModes);
        const VkExtent2D extent = ChooseSwapExtent(swapchainSupport.Capabilities, m_Window);

        uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;

        const QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        const uint32_t queueFamilyIndices[] =
        {
            indices.GraphicsFamily.value(),
            indices.PresentFamily.value()
        };

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (indices.GraphicsFamily != indices.PresentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        const VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);

        if (result != VK_SUCCESS) {
            Log::Error(std::format("Failed to create Vulkan swapchain. VkResult={}", static_cast<int>(result)));
            return false;
        }

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_SwapchainImageCount, nullptr);

        m_SwapchainImages = std::make_unique<VkImage[]>(m_SwapchainImageCount);

        vkGetSwapchainImagesKHR(
            m_Device,
            m_Swapchain,
            &m_SwapchainImageCount,
            m_SwapchainImages.get()
        );

        m_SwapchainImageFormat = surfaceFormat.format;
        m_SwapchainExtent = extent;

        Log::Info(std::format(
            "Vulkan swapchain created. Images: {}, Extent: {}x{}",
            m_SwapchainImageCount,
            m_SwapchainExtent.width,
            m_SwapchainExtent.height
        ));

        return true;
    }

    bool VulkanContext::CreateRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependency.srcAccessMask = 0;

        dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;

        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        const VkResult result = vkCreateRenderPass(
            m_Device,
            &renderPassInfo,
            nullptr,
            &m_RenderPass
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan render pass. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan render pass created.");
        return true;
    }

    bool VulkanContext::CreateImageViews() {
        m_SwapchainImageViews = std::make_unique<VkImageView[]>(m_SwapchainImageCount);

        for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
            m_SwapchainImageViews[i] = VK_NULL_HANDLE;

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_SwapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_SwapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            const VkResult result = vkCreateImageView(
                m_Device,
                &createInfo,
                nullptr,
                &m_SwapchainImageViews[i]
            );

            if (result != VK_SUCCESS) {
                Log::Error(std::format(
                    "Failed to create swapchain image view {}. VkResult={}",
                    i,
                    static_cast<int>(result)
                ));

                return false;
            }
        }

        Log::Info(std::format("Vulkan swapchain image views created. Count: {}", m_SwapchainImageCount));
        return true;
    }

    SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device) const {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

        if (formatCount > 0) {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

        if (presentModeCount > 0) {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount,
                                                      details.PresentModes.data());
        }

        return details;
    }

    QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                indices.GraphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

            if (presentSupport == VK_TRUE) {
                indices.PresentFamily = i;
            }

            if (indices.IsComplete()) {
                break;
            }
        }

        return indices;
    }

    bool VulkanContext::CreateDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerLayoutBinding;

        const VkResult result = vkCreateDescriptorSetLayout(
            m_Device,
            &layoutInfo,
            nullptr,
            &m_TextureDescriptorSetLayout
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan descriptor set layout. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan descriptor set layout created.");
        return true;
    }

    bool VulkanContext::CreateGraphicsPipeline() {
        const std::vector<char> vertexShaderCode =
                ReadBinaryFile("sandbox/shaders/fullscreen_image.vert.spv");

        const std::vector<char> fragmentShaderCode =
                ReadBinaryFile("sandbox/shaders/fullscreen_image.frag.spv");

        VkShaderModule vertexShaderModule =
                CreateShaderModuleFromBytes(m_Device, vertexShaderCode);

        VkShaderModule fragmentShaderModule =
                CreateShaderModuleFromBytes(m_Device, fragmentShaderCode);

        if (vertexShaderModule == VK_NULL_HANDLE ||
            fragmentShaderModule == VK_NULL_HANDLE) {
            if (vertexShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            }

            if (fragmentShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            }

            return false;
        }

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(float) * 4 + sizeof(int);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_TextureDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result = vkCreatePipelineLayout(
            m_Device,
            &pipelineLayoutInfo,
            nullptr,
            &m_PipelineLayout
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan pipeline layout. VkResult={}",
                static_cast<int>(result)
            ));

            vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.renderPass = m_RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        result = vkCreateGraphicsPipelines(
            m_Device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_GraphicsPipeline
        );

        vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan graphics pipeline. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan fullscreen image graphics pipeline created.");
        return true;
    }

    bool VulkanContext::CreateRect2DPipeline() {
        const std::vector<char> vertexShaderCode =
                ReadBinaryFile("sandbox/shaders/rect2d.vert.spv");

        const std::vector<char> fragmentShaderCode =
                ReadBinaryFile("sandbox/shaders/rect2d.frag.spv");

        VkShaderModule vertexShaderModule =
                CreateShaderModuleFromBytes(m_Device, vertexShaderCode);

        VkShaderModule fragmentShaderModule =
                CreateShaderModuleFromBytes(m_Device, fragmentShaderCode);

        if (vertexShaderModule == VK_NULL_HANDLE ||
            fragmentShaderModule == VK_NULL_HANDLE) {
            if (vertexShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            }

            if (fragmentShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            }

            return false;
        }

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Rect2DVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Rect2DVertex, X);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Rect2DVertex, R);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        struct Rect2DPushConstants {
            float ViewportWidth;
            float ViewportHeight;
        };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(Rect2DPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result = vkCreatePipelineLayout(
            m_Device,
            &pipelineLayoutInfo,
            nullptr,
            &m_Rect2DPipelineLayout
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan Rect2D pipeline layout. VkResult={}",
                static_cast<int>(result)
            ));

            vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_Rect2DPipelineLayout;
        pipelineInfo.renderPass = m_RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        result = vkCreateGraphicsPipelines(
            m_Device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_Rect2DPipeline
        );

        vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan Rect2D graphics pipeline. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan Rect2D graphics pipeline created.");
        return true;
    }

    bool VulkanContext::UpdateDraw2DVertexBuffer() {
        m_Rect2DVertexCount = 0;
        m_Draw2DLayerBatches.clear();

        if (m_Draw2D == nullptr) {
            return true;
        }

        const std::vector<RectCommand> &rects =
                m_Draw2D->GetRectCommands();

        const std::vector<PolygonCommand> &polygons =
                m_Draw2D->GetPolygonCommands();

        if (rects.empty() && polygons.empty()) {
            return true;
        }

        std::size_t estimatedVertexCount = rects.size() * 6;

        for (const PolygonCommand &polygon: polygons) {
            const int pointCount = std::clamp(polygon.Points, 3, 512);
            estimatedVertexCount += static_cast<std::size_t>(pointCount) * 3;
        }

        std::vector<Rect2DVertex> vertices;
        vertices.reserve(estimatedVertexCount);

        std::vector<int> layers;

        for (const RectCommand &rect: rects) {
            layers.push_back(rect.Layer);
        }

        for (const PolygonCommand &polygon: polygons) {
            layers.push_back(polygon.Layer);
        }

        std::ranges::sort(layers);

        layers.erase(
            std::unique(layers.begin(), layers.end()),
            layers.end()
        );

        for (const int layer: layers) {
            const uint32_t firstVertex =
                    static_cast<uint32_t>(vertices.size());

            for (const RectCommand &rect: rects) {
                if (rect.Layer != layer) {
                    continue;
                }

                const float x0 = rect.X;
                const float y0 = rect.Y;
                const float x1 = rect.X + rect.Width;
                const float y1 = rect.Y + rect.Height;

                const Rect2DVertex topLeft =
                {
                    x0,
                    y0,
                    rect.R,
                    rect.G,
                    rect.B,
                    rect.A
                };

                const Rect2DVertex topRight =
                {
                    x1,
                    y0,
                    rect.R,
                    rect.G,
                    rect.B,
                    rect.A
                };

                const Rect2DVertex bottomRight =
                {
                    x1,
                    y1,
                    rect.R,
                    rect.G,
                    rect.B,
                    rect.A
                };

                const Rect2DVertex bottomLeft =
                {
                    x0,
                    y1,
                    rect.R,
                    rect.G,
                    rect.B,
                    rect.A
                };

                vertices.push_back(topLeft);
                vertices.push_back(topRight);
                vertices.push_back(bottomRight);

                vertices.push_back(topLeft);
                vertices.push_back(bottomRight);
                vertices.push_back(bottomLeft);
            }

            for (const PolygonCommand &polygon: polygons) {
                if (polygon.Layer != layer) {
                    continue;
                }

                const int pointCount = std::clamp(polygon.Points, 3, 512);

                const float rotationRadians =
                        polygon.RotationDegrees * PI / 180.0f;

                for (int i = 0; i < pointCount; i++) {
                    const int nextIndex = (i + 1) % pointCount;

                    const float angle0 =
                            rotationRadians +
                            (static_cast<float>(i) / static_cast<float>(pointCount)) *
                            PI *
                            2.0f;

                    const float angle1 =
                            rotationRadians +
                            (static_cast<float>(nextIndex) / static_cast<float>(pointCount)) *
                            PI *
                            2.0f;

                    const float x0 =
                            polygon.CenterX + std::cos(angle0) * polygon.Radius;

                    const float y0 =
                            polygon.CenterY + std::sin(angle0) * polygon.Radius;

                    const float x1 =
                            polygon.CenterX + std::cos(angle1) * polygon.Radius;

                    const float y1 =
                            polygon.CenterY + std::sin(angle1) * polygon.Radius;

                    const Rect2DVertex center =
                    {
                        polygon.CenterX,
                        polygon.CenterY,
                        polygon.R,
                        polygon.G,
                        polygon.B,
                        polygon.A
                    };

                    const Rect2DVertex outer0 =
                    {
                        x0,
                        y0,
                        polygon.R,
                        polygon.G,
                        polygon.B,
                        polygon.A
                    };

                    const Rect2DVertex outer1 =
                    {
                        x1,
                        y1,
                        polygon.R,
                        polygon.G,
                        polygon.B,
                        polygon.A
                    };

                    vertices.push_back(center);
                    vertices.push_back(outer0);
                    vertices.push_back(outer1);
                }
            }

            const uint32_t vertexCount =
                    static_cast<uint32_t>(vertices.size()) - firstVertex;

            if (vertexCount > 0) {
                Draw2DLayerBatch batch{};
                batch.Layer = layer;
                batch.FirstVertex = firstVertex;
                batch.VertexCount = vertexCount;

                m_Draw2DLayerBatches.push_back(batch);
            }
        }

        const VkDeviceSize requiredSize =
                static_cast<VkDeviceSize>(vertices.size() * sizeof(Rect2DVertex));

        if (requiredSize == 0) {
            return true;
        }

        if (m_Rect2DVertexBuffer == VK_NULL_HANDLE ||
            m_Rect2DVertexBufferSize < requiredSize) {
            DestroyDraw2DResources();

            if (!CreateBuffer(
                requiredSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_Rect2DVertexBuffer,
                m_Rect2DVertexBufferMemory)) {
                return false;
            }

            m_Rect2DVertexBufferSize = requiredSize;
        }

        void *mappedMemory = nullptr;

        const VkResult mapResult = vkMapMemory(
            m_Device,
            m_Rect2DVertexBufferMemory,
            0,
            requiredSize,
            0,
            &mappedMemory
        );

        if (mapResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to map Draw2D vertex buffer memory. VkResult={}",
                static_cast<int>(mapResult)
            ));

            return false;
        }

        std::memcpy(
            mappedMemory,
            vertices.data(),
            static_cast<std::size_t>(requiredSize)
        );

        vkUnmapMemory(m_Device, m_Rect2DVertexBufferMemory);

        m_Rect2DVertexCount = static_cast<uint32_t>(vertices.size());
        return true;
    }

    void VulkanContext::DestroyDraw2DResources() {
        if (m_Device == VK_NULL_HANDLE) {
            return;
        }

        if (m_Rect2DVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, m_Rect2DVertexBuffer, nullptr);
            m_Rect2DVertexBuffer = VK_NULL_HANDLE;
        }

        if (m_Rect2DVertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_Rect2DVertexBufferMemory, nullptr);
            m_Rect2DVertexBufferMemory = VK_NULL_HANDLE;
        }

        m_Rect2DVertexBufferSize = 0;
        m_Rect2DVertexCount = 0;
    }

    bool VulkanContext::CreateDescriptorPool() {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 256;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 256;

        const VkResult result = vkCreateDescriptorPool(
            m_Device,
            &poolInfo,
            nullptr,
            &m_DescriptorPool
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan descriptor pool. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan descriptor pool created.");
        return true;
    }

    void VulkanContext::SetImageScaleMode(int scaleMode) {
        if (scaleMode < 0 || scaleMode > 3) {
            Log::Error(std::format(
                "Invalid image scale mode: {}. Expected 0-3.",
                scaleMode
            ));

            return;
        }

        m_ImageScaleMode = scaleMode;

        Log::Info(std::format(
            "Image scale mode changed to: {}",
            m_ImageScaleMode
        ));
    }

    bool VulkanContext::CreateDescriptorSet() {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_DescriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_TextureDescriptorSetLayout;

        const VkResult result = vkAllocateDescriptorSets(
            m_Device,
            &allocateInfo,
            &m_TextureDescriptorSet
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to allocate Vulkan texture descriptor set. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan texture descriptor set allocated.");
        return true;
    }

    bool VulkanContext::UpdateTextureDescriptor() {
        if (m_TextureImageView == VK_NULL_HANDLE ||
            m_TextureSampler == VK_NULL_HANDLE ||
            m_TextureDescriptorSet == VK_NULL_HANDLE) {
            Log::Error("Cannot update texture descriptor because texture resources are incomplete.");
            return false;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_TextureImageView;
        imageInfo.sampler = m_TextureSampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_TextureDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            m_Device,
            1,
            &descriptorWrite,
            0,
            nullptr
        );

        return true;
    }

    void VulkanContext::SetSpriteManager(const SpriteManager *spriteManager) {
        m_SpriteManager = spriteManager;
    }

    VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats
    ) {
        for (const VkSurfaceFormatKHR &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    bool VulkanContext::EnsureSpriteTexturesForFrame() {
        if (m_SpriteManager == nullptr) {
            return true;
        }

        const std::vector<SpriteDrawCommand> &drawCommands =
                m_SpriteManager->GetDrawCommands();

        for (const SpriteDrawCommand &command: drawCommands) {
            if (command.Handle == 0) {
                continue;
            }

            if (m_SpriteTextures.contains(command.Handle)) {
                continue;
            }

            const SpriteResource *sprite =
                    m_SpriteManager->GetSprite(command.Handle);

            if (sprite == nullptr) {
                Log::Error(std::format(
                    "Cannot upload sprite texture. Missing sprite handle={}",
                    command.Handle
                ));

                return false;
            }

            if (!sprite->Loaded || !sprite->Image.IsValid()) {
                Log::Error(std::format(
                    "Cannot upload sprite texture. Sprite image is invalid. Handle={} Path={}",
                    sprite->Handle,
                    sprite->Path
                ));

                return false;
            }

            if (!UploadSpriteTexture(*sprite)) {
                return false;
            }
        }

        return true;
    }

    bool VulkanContext::UploadSpriteTexture(const SpriteResource &sprite) {
        if (m_Device == VK_NULL_HANDLE) {
            Log::Error("Cannot upload sprite texture because Vulkan device is null.");
            return false;
        }

        const ImageData &image = sprite.Image;

        const VkDeviceSize imageSize =
                static_cast<VkDeviceSize>(image.Width) *
                static_cast<VkDeviceSize>(image.Height) *
                static_cast<VkDeviceSize>(image.DesiredChannels);

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        if (!CreateBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory)) {
            return false;
        }

        void *mappedMemory = nullptr;

        const VkResult mapResult = vkMapMemory(
            m_Device,
            stagingBufferMemory,
            0,
            imageSize,
            0,
            &mappedMemory
        );

        if (mapResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to map sprite staging buffer memory. VkResult={}",
                static_cast<int>(mapResult)
            ));

            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        std::memcpy(
            mappedMemory,
            image.Pixels.data(),
            static_cast<std::size_t>(imageSize)
        );

        vkUnmapMemory(m_Device, stagingBufferMemory);

        SpriteGpuTexture texture{};
        texture.Width = static_cast<uint32_t>(image.Width);
        texture.Height = static_cast<uint32_t>(image.Height);

        if (!CreateImage(
            texture.Width,
            texture.Height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            texture.Image,
            texture.ImageMemory)) {
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!TransitionImageLayout(
            texture.Image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
            DestroySpriteTexture(texture);
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!CopyBufferToImage(
            stagingBuffer,
            texture.Image,
            texture.Width,
            texture.Height)) {
            DestroySpriteTexture(texture);
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!TransitionImageLayout(
            texture.Image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            DestroySpriteTexture(texture);
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
        vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

        if (!CreateImageView(
            texture.Image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            texture.ImageView)) {
            DestroySpriteTexture(texture);
            return false;
        }

        if (!CreateSpriteSampler(texture.Sampler)) {
            DestroySpriteTexture(texture);
            return false;
        }

        if (!CreateSpriteDescriptorSet(texture)) {
            DestroySpriteTexture(texture);
            return false;
        }

        m_SpriteTextures.emplace(sprite.Handle, texture);

        Log::Info(std::format(
            "Sprite texture uploaded | Handle={} | Path={} | Size={}x{} | Bytes={}",
            sprite.Handle,
            sprite.Path,
            texture.Width,
            texture.Height,
            image.Pixels.size()
        ));

        return true;
    }

    bool VulkanContext::CreateSpriteSampler(VkSampler &sampler) const {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        const VkResult result = vkCreateSampler(
            m_Device,
            &samplerInfo,
            nullptr,
            &sampler
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create sprite sampler. VkResult={}",
                static_cast<int>(result)
            ));

            sampler = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    void VulkanContext::BuildRenderQueue() {
        m_RenderQueue.clear();

        for (std::size_t i = 0; i < m_Draw2DLayerBatches.size(); i++) {
            const Draw2DLayerBatch &batch = m_Draw2DLayerBatches[i];

            RenderQueueEntry entry{};
            entry.Type = RenderQueueEntryType::Draw2D;
            entry.Layer = batch.Layer;
            entry.BatchIndex = i;
            entry.Order = 0;

            m_RenderQueue.push_back(entry);
        }

        for (std::size_t i = 0; i < m_Sprite2DBatches.size(); i++) {
            const Sprite2DBatch &batch = m_Sprite2DBatches[i];

            RenderQueueEntry entry{};
            entry.Type = RenderQueueEntryType::Sprite2D;
            entry.Layer = batch.Layer;
            entry.BatchIndex = i;
            entry.Order = 1;

            m_RenderQueue.push_back(entry);
        }

        std::ranges::stable_sort(
            m_RenderQueue,
            [](const RenderQueueEntry &a, const RenderQueueEntry &b) {
                if (a.Layer != b.Layer) {
                    return a.Layer < b.Layer;
                }

                return a.Order < b.Order;
            }
        );
    }

    bool VulkanContext::UpdateSprite2DVertexBuffer(
        const std::vector<SpriteDrawCommand> &commands
    ) {
        m_Sprite2DVertexCount = 0;
        m_Sprite2DDrawCalls.clear();
        m_Sprite2DBatches.clear();

        if (commands.empty()) {
            return true;
        }

        std::vector<SpriteDrawCommand> sortedCommands = commands;

        std::ranges::stable_sort(
            sortedCommands,
            [](const SpriteDrawCommand &a, const SpriteDrawCommand &b) {
                return a.Layer < b.Layer;
            }
        );

        std::vector<Sprite2DVertex> vertices;
        vertices.reserve(sortedCommands.size() * 6);

        for (const SpriteDrawCommand &command: sortedCommands) {
            if (!m_SpriteTextures.contains(command.Handle)) {
                continue;
            }

            const uint32_t firstVertex =
                    static_cast<uint32_t>(vertices.size());

            const float centerX = command.X + command.Width * 0.5f;
            const float centerY = command.Y + command.Height * 0.5f;

            const float halfWidth = command.Width * 0.5f;
            const float halfHeight = command.Height * 0.5f;

            const float rotationRadians =
                    command.RotationDegrees * PI / 180.0f;

            const float cosRotation = std::cos(rotationRadians);
            const float sinRotation = std::sin(rotationRadians);

            auto rotatePoint =
                    [&](float localX, float localY) -> std::pair<float, float> {
                const float rotatedX =
                        localX * cosRotation -
                        localY * sinRotation;

                const float rotatedY =
                        localX * sinRotation +
                        localY * cosRotation;

                return
                {
                    centerX + rotatedX,
                    centerY + rotatedY
                };
            };

            const auto [topLeftX, topLeftY] =
                    rotatePoint(-halfWidth, -halfHeight);

            const auto [topRightX, topRightY] =
                    rotatePoint(halfWidth, -halfHeight);

            const auto [bottomRightX, bottomRightY] =
                    rotatePoint(halfWidth, halfHeight);

            const auto [bottomLeftX, bottomLeftY] =
                    rotatePoint(-halfWidth, halfHeight);

            const Sprite2DVertex topLeft =
            {
                topLeftX,
                topLeftY,
                0.0f,
                1.0f,
                command.R,
                command.G,
                command.B,
                command.A
            };

            const Sprite2DVertex topRight =
            {
                topRightX,
                topRightY,
                1.0f,
                1.0f,
                command.R,
                command.G,
                command.B,
                command.A
            };

            const Sprite2DVertex bottomRight =
            {
                bottomRightX,
                bottomRightY,
                1.0f,
                0.0f,
                command.R,
                command.G,
                command.B,
                command.A
            };

            const Sprite2DVertex bottomLeft =
            {
                bottomLeftX,
                bottomLeftY,
                0.0f,
                0.0f,
                command.R,
                command.G,
                command.B,
                command.A
            };

            vertices.push_back(topLeft);
            vertices.push_back(topRight);
            vertices.push_back(bottomRight);

            vertices.push_back(topLeft);
            vertices.push_back(bottomRight);
            vertices.push_back(bottomLeft);

            Sprite2DDrawCall drawCall{};
            drawCall.Layer = command.Layer;
            drawCall.Handle = command.Handle;
            drawCall.FirstVertex = firstVertex;
            drawCall.VertexCount = 6;

            m_Sprite2DDrawCalls.push_back(drawCall);

            if (!m_Sprite2DBatches.empty()) {
                Sprite2DBatch &lastBatch = m_Sprite2DBatches.back();

                const bool canMergeWithLastBatch =
                        lastBatch.Layer == command.Layer &&
                        lastBatch.Handle == command.Handle &&
                        lastBatch.FirstVertex + lastBatch.VertexCount == firstVertex;

                if (canMergeWithLastBatch) {
                    lastBatch.VertexCount += 6;
                } else {
                    Sprite2DBatch batch{};
                    batch.Layer = command.Layer;
                    batch.Handle = command.Handle;
                    batch.FirstVertex = firstVertex;
                    batch.VertexCount = 6;

                    m_Sprite2DBatches.push_back(batch);
                }
            } else {
                Sprite2DBatch batch{};
                batch.Layer = command.Layer;
                batch.Handle = command.Handle;
                batch.FirstVertex = firstVertex;
                batch.VertexCount = 6;

                m_Sprite2DBatches.push_back(batch);
            }
        }

        const VkDeviceSize requiredSize =
                static_cast<VkDeviceSize>(vertices.size() * sizeof(Sprite2DVertex));

        if (requiredSize == 0) {
            return true;
        }

        if (m_Sprite2DVertexBuffer == VK_NULL_HANDLE ||
            m_Sprite2DVertexBufferSize < requiredSize) {
            DestroySprite2DResources();

            if (!CreateBuffer(
                requiredSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_Sprite2DVertexBuffer,
                m_Sprite2DVertexBufferMemory)) {
                return false;
            }

            m_Sprite2DVertexBufferSize = requiredSize;
        }

        void *mappedMemory = nullptr;

        const VkResult mapResult = vkMapMemory(
            m_Device,
            m_Sprite2DVertexBufferMemory,
            0,
            requiredSize,
            0,
            &mappedMemory
        );

        if (mapResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to map Sprite2D vertex buffer memory. VkResult={}",
                static_cast<int>(mapResult)
            ));

            return false;
        }

        std::memcpy(
            mappedMemory,
            vertices.data(),
            static_cast<std::size_t>(requiredSize)
        );

        vkUnmapMemory(m_Device, m_Sprite2DVertexBufferMemory);

        m_Sprite2DVertexCount = static_cast<uint32_t>(vertices.size());

        return true;
    }

    void VulkanContext::DestroySpriteTexture(SpriteGpuTexture &texture) {
        if (m_Device == VK_NULL_HANDLE) {
            return;
        }

        if (texture.Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device, texture.Sampler, nullptr);
            texture.Sampler = VK_NULL_HANDLE;
        }

        if (texture.ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_Device, texture.ImageView, nullptr);
            texture.ImageView = VK_NULL_HANDLE;
        }

        if (texture.Image != VK_NULL_HANDLE) {
            vkDestroyImage(m_Device, texture.Image, nullptr);
            texture.Image = VK_NULL_HANDLE;
        }

        if (texture.ImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, texture.ImageMemory, nullptr);
            texture.ImageMemory = VK_NULL_HANDLE;
        }

        texture.Width = 0;
        texture.Height = 0;
    }

    void VulkanContext::DestroySpriteTextures() {
        for (auto &[handle, texture]: m_SpriteTextures) {
            DestroySpriteTexture(texture);
        }

        m_SpriteTextures.clear();
    }

    bool VulkanContext::CreateSpriteDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerLayoutBinding;

        const VkResult result = vkCreateDescriptorSetLayout(
            m_Device,
            &layoutInfo,
            nullptr,
            &m_SpriteDescriptorSetLayout
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create sprite descriptor set layout. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan Sprite2D descriptor set layout created.");
        return true;
    }

    bool VulkanContext::CreateSprite2DPipeline() {
        const std::vector<char> vertexShaderCode =
                ReadBinaryFile("sandbox/shaders/sprite2d.vert.spv");

        const std::vector<char> fragmentShaderCode =
                ReadBinaryFile("sandbox/shaders/sprite2d.frag.spv");

        VkShaderModule vertexShaderModule =
                CreateShaderModuleFromBytes(m_Device, vertexShaderCode);

        VkShaderModule fragmentShaderModule =
                CreateShaderModuleFromBytes(m_Device, fragmentShaderCode);

        if (vertexShaderModule == VK_NULL_HANDLE ||
            fragmentShaderModule == VK_NULL_HANDLE) {
            if (vertexShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            }

            if (fragmentShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            }

            return false;
        }

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Sprite2DVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Sprite2DVertex, X);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Sprite2DVertex, U);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Sprite2DVertex, R);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions =
                attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor =
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor =
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType =
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        struct Sprite2DPushConstants {
            float ViewportWidth;
            float ViewportHeight;
        };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(Sprite2DPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_SpriteDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result = vkCreatePipelineLayout(
            m_Device,
            &pipelineLayoutInfo,
            nullptr,
            &m_Sprite2DPipelineLayout
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Sprite2D pipeline layout. VkResult={}",
                static_cast<int>(result)
            ));

            vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
            vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_Sprite2DPipelineLayout;
        pipelineInfo.renderPass = m_RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        result = vkCreateGraphicsPipelines(
            m_Device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_Sprite2DPipeline
        );

        vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Sprite2D graphics pipeline. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        Log::Info("Vulkan Sprite2D graphics pipeline created.");
        return true;
    }

    bool VulkanContext::CreateSpriteDescriptorSet(SpriteGpuTexture &texture) {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_DescriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_SpriteDescriptorSetLayout;

        const VkResult result = vkAllocateDescriptorSets(
            m_Device,
            &allocateInfo,
            &texture.DescriptorSet
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to allocate sprite descriptor set. VkResult={}",
                static_cast<int>(result)
            ));

            texture.DescriptorSet = VK_NULL_HANDLE;
            return false;
        }

        return UpdateSpriteDescriptorSet(texture);
    }

    bool VulkanContext::UpdateSpriteDescriptorSet(
        const SpriteGpuTexture &texture
    ) const {
        if (texture.ImageView == VK_NULL_HANDLE ||
            texture.Sampler == VK_NULL_HANDLE ||
            texture.DescriptorSet == VK_NULL_HANDLE) {
            return false;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture.ImageView;
        imageInfo.sampler = texture.Sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = texture.DescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            m_Device,
            1,
            &descriptorWrite,
            0,
            nullptr
        );

        return true;
    }

    void VulkanContext::DestroySprite2DResources() {
        if (m_Device == VK_NULL_HANDLE) {
            return;
        }

        if (m_Sprite2DVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, m_Sprite2DVertexBuffer, nullptr);
            m_Sprite2DVertexBuffer = VK_NULL_HANDLE;
        }

        if (m_Sprite2DVertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_Sprite2DVertexBufferMemory, nullptr);
            m_Sprite2DVertexBufferMemory = VK_NULL_HANDLE;
        }

        m_Sprite2DVertexBufferSize = 0;
        m_Sprite2DVertexCount = 0;
    }

    VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes
    ) {
        for (const VkPresentModeKHR &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanContext::ChooseSwapExtent(
        const VkSurfaceCapabilitiesKHR &capabilities,
        GLFWwindow *window
    ) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );

        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return actualExtent;
    }

    void VulkanContext::SetClearColor(float r, float g, float b, float a) {
        m_ClearColor = {{r, g, b, a}};

        Log::Info(std::format(
            "Renderer clear color changed to: {}, {}, {}, {}",
            r,
            g,
            b,
            a
        ));
    }

    bool VulkanContext::SetTextureFromImage(const ImageData &image) {
        if (!image.IsValid()) {
            Log::Error("Cannot upload invalid image to Vulkan texture.");
            return false;
        }

        if (m_Device == VK_NULL_HANDLE) {
            Log::Error("Cannot upload texture because Vulkan device is null.");
            return false;
        }

        /*
            v0.01 safety move.

            We wait for the GPU before replacing texture resources. Later we will
            move this to a better deferred-destruction system.
        */
        vkDeviceWaitIdle(m_Device);

        DestroyTextureResources();
        DestroySpriteTextures();

        const VkDeviceSize imageSize =
                static_cast<VkDeviceSize>(image.Width) *
                static_cast<VkDeviceSize>(image.Height) *
                static_cast<VkDeviceSize>(image.DesiredChannels);

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        if (!CreateBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory)) {
            return false;
        }

        void *mappedMemory = nullptr;

        const VkResult mapResult = vkMapMemory(
            m_Device,
            stagingBufferMemory,
            0,
            imageSize,
            0,
            &mappedMemory
        );

        if (mapResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to map texture staging buffer memory. VkResult={}",
                static_cast<int>(mapResult)
            ));

            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        std::memcpy(mappedMemory, image.Pixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(m_Device, stagingBufferMemory);

        if (!CreateImage(
            static_cast<uint32_t>(image.Width),
            static_cast<uint32_t>(image.Height),
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_TextureImage,
            m_TextureImageMemory)) {
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!TransitionImageLayout(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!CopyBufferToImage(
            stagingBuffer,
            m_TextureImage,
            static_cast<uint32_t>(image.Width),
            static_cast<uint32_t>(image.Height))) {
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        if (!TransitionImageLayout(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
            return false;
        }

        vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
        vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

        if (!CreateImageView(
            m_TextureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            m_TextureImageView)) {
            return false;
        }

        if (!CreateTextureSampler()) {
            return false;
        }

        if (m_TextureDescriptorSet == VK_NULL_HANDLE) {
            if (!CreateDescriptorSet()) {
                return false;
            }
        }

        if (!UpdateTextureDescriptor()) {
            return false;
        }

        m_TextureWidth = static_cast<uint32_t>(image.Width);
        m_TextureHeight = static_cast<uint32_t>(image.Height);

        Log::Info(std::format(
            "Vulkan texture uploaded. Size: {}x{}, Bytes: {}",
            m_TextureWidth,
            m_TextureHeight,
            image.Pixels.size()
        ));

        return true;
    }

    void VulkanContext::DestroyTextureResources() {
        if (m_Device == VK_NULL_HANDLE) {
            return;
        }

        if (m_TextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_Device, m_TextureSampler, nullptr);
            m_TextureSampler = VK_NULL_HANDLE;
        }

        if (m_TextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
            m_TextureImageView = VK_NULL_HANDLE;
        }

        if (m_TextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(m_Device, m_TextureImage, nullptr);
            m_TextureImage = VK_NULL_HANDLE;
        }

        if (m_TextureImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
            m_TextureImageMemory = VK_NULL_HANDLE;
        }

        m_TextureWidth = 0;
        m_TextureHeight = 0;
    }

    uint32_t VulkanContext::FindMemoryType(
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    ) const {
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(
            m_PhysicalDevice,
            &memoryProperties
        );

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            const bool typeMatches =
                    (typeFilter & (1 << i)) != 0;

            const bool propertiesMatch =
                    (memoryProperties.memoryTypes[i].propertyFlags & properties) ==
                    properties;

            if (typeMatches && propertiesMatch) {
                return i;
            }
        }

        Log::Error("Failed to find suitable Vulkan memory type.");
        return UINT32_MAX;
    }

    bool VulkanContext::CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &bufferMemory
    ) const {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        const VkResult bufferResult = vkCreateBuffer(
            m_Device,
            &bufferInfo,
            nullptr,
            &buffer
        );

        if (bufferResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan buffer. VkResult={}",
                static_cast<int>(bufferResult)
            ));

            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(
            m_Device,
            buffer,
            &memoryRequirements
        );

        const uint32_t memoryTypeIndex =
                FindMemoryType(memoryRequirements.memoryTypeBits, properties);

        if (memoryTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;

        const VkResult allocateResult = vkAllocateMemory(
            m_Device,
            &allocateInfo,
            nullptr,
            &bufferMemory
        );

        if (allocateResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to allocate Vulkan buffer memory. VkResult={}",
                static_cast<int>(allocateResult)
            ));

            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return false;
        }

        vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
        return true;
    }

    bool VulkanContext::CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage &image,
        VkDeviceMemory &imageMemory
    ) const {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        const VkResult imageResult = vkCreateImage(
            m_Device,
            &imageInfo,
            nullptr,
            &image
        );

        if (imageResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan image. VkResult={}",
                static_cast<int>(imageResult)
            ));

            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(
            m_Device,
            image,
            &memoryRequirements
        );

        const uint32_t memoryTypeIndex =
                FindMemoryType(memoryRequirements.memoryTypeBits, properties);

        if (memoryTypeIndex == UINT32_MAX) {
            vkDestroyImage(m_Device, image, nullptr);
            image = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;

        const VkResult allocateResult = vkAllocateMemory(
            m_Device,
            &allocateInfo,
            nullptr,
            &imageMemory
        );

        if (allocateResult != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to allocate Vulkan image memory. VkResult={}",
                static_cast<int>(allocateResult)
            ));

            vkDestroyImage(m_Device, image, nullptr);
            image = VK_NULL_HANDLE;
            return false;
        }

        vkBindImageMemory(m_Device, image, imageMemory, 0);
        return true;
    }

    bool VulkanContext::CreateImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        VkImageView &imageView
    ) const {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;

        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        const VkResult result = vkCreateImageView(
            m_Device,
            &viewInfo,
            nullptr,
            &imageView
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan image view. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        return true;
    }

    VkCommandBuffer VulkanContext::BeginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        vkAllocateCommandBuffers(
            m_Device,
            &allocateInfo,
            &commandBuffer
        );

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(
            m_GraphicsQueue,
            1,
            &submitInfo,
            VK_NULL_HANDLE
        );

        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(
            m_Device,
            m_CommandPool,
            1,
            &commandBuffer
        );
    }

    bool VulkanContext::TransitionImageLayout(
        VkImage image,
        VkFormat,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ) {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            Log::Error("Unsupported Vulkan image layout transition.");
            return false;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        EndSingleTimeCommands(commandBuffer);
        return true;
    }

    bool VulkanContext::CopyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    ) {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        EndSingleTimeCommands(commandBuffer);
        return true;
    }

    bool VulkanContext::CreateTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        const VkResult result = vkCreateSampler(
            m_Device,
            &samplerInfo,
            nullptr,
            &m_TextureSampler
        );

        if (result != VK_SUCCESS) {
            Log::Error(std::format(
                "Failed to create Vulkan texture sampler. VkResult={}",
                static_cast<int>(result)
            ));

            return false;
        }

        return true;
    }

    void VulkanContext::SetDraw2D(const Draw2D *draw2D) {
        m_Draw2D = draw2D;
    }
}
