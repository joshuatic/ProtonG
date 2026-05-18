#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_map>

#include "ProtonImageLoader.hpp"
#include "ProtonDraw2D.hpp"
#include "ProtonSpriteManager.hpp"

namespace Proton {
    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;

        bool IsComplete() const {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities{};
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct RenderLayerEntry {
        int Layer = 0;
    };

    class VulkanContext {
    public:
        VulkanContext() = default;

        ~VulkanContext();

        bool Init(GLFWwindow *window);

        void DrawFrame();

        void NotifyFramebufferResized();

        void Shutdown();

        void SetClearColor(float r, float g, float b, float a);

        bool SetTextureFromImage(const ImageData &image);

        void SetImageScaleMode(int scaleMode);

        void SetDraw2D(const Draw2D *draw2D);

        void SetSpriteManager(const SpriteManager *spriteManager);

    private:
        struct SpriteGpuTexture {
            VkImage Image = VK_NULL_HANDLE;
            VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
            VkImageView ImageView = VK_NULL_HANDLE;
            VkSampler Sampler = VK_NULL_HANDLE;

            VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

            uint32_t Width = 0;
            uint32_t Height = 0;
        };

        struct Draw2DLayerBatch {
            int Layer = 0;
            uint32_t FirstVertex = 0;
            uint32_t VertexCount = 0;
        };

        struct Sprite2DDrawCall {
            int Layer = 0;
            SpriteHandle Handle = 0;
            uint32_t FirstVertex = 0;
            uint32_t VertexCount = 6;
        };

        struct Sprite2DBatch {
            int Layer = 0;
            SpriteHandle Handle = 0;
            uint32_t FirstVertex = 0;
            uint32_t VertexCount = 0;
        };

        enum class RenderQueueEntryType {
            Draw2D,
            Sprite2D
        };

        struct RenderQueueEntry {
            RenderQueueEntryType Type = RenderQueueEntryType::Draw2D;

            int Layer = 0;

            /*
                Index into:
                - m_Draw2DLayerBatches when Type == Draw2D
                - m_Sprite2DBatches when Type == Sprite2D
            */
            std::size_t BatchIndex = 0;

            /*
                Same-layer ordering:
                Lower value draws first.

                Current rule:
                - Shapes first
                - Sprites second
            */
            int Order = 0;
        };

        const SpriteManager *m_SpriteManager = nullptr;
        std::unordered_map<SpriteHandle, SpriteGpuTexture> m_SpriteTextures;

        std::vector<Sprite2DBatch> m_Sprite2DBatches;
        std::vector<RenderQueueEntry> m_RenderQueue;

        bool CreateInstance();

        bool CreateSurface(GLFWwindow *window);

        bool PickPhysicalDevice();

        bool CreateLogicalDevice();

        bool CreateSwapchain();

        bool CreateImageViews();

        bool CreateRenderPass();

        bool CreateFramebuffers();

        bool CreateCommandPool();

        bool CreateCommandBuffers();

        bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        bool CreateSyncObjects();

        bool CreateRect2DPipeline();

        bool UpdateDraw2DVertexBuffer();

        void DestroyDraw2DResources();

        bool EnsureSpriteTexturesForFrame();

        bool UploadSpriteTexture(const SpriteResource &sprite);

        void DestroySpriteTexture(SpriteGpuTexture &texture);

        void DestroySpriteTextures();

        bool CreateSpriteDescriptorSetLayout();

        bool CreateSprite2DPipeline();

        bool CreateSpriteDescriptorSet(SpriteGpuTexture &texture);

        bool UpdateSpriteDescriptorSet(const SpriteGpuTexture &texture) const;

        bool UpdateSprite2DVertexBuffer(const std::vector<SpriteDrawCommand> &commands);

        void DestroySprite2DResources();

        bool CreateSpriteSampler(VkSampler &sampler) const;

        void BuildRenderQueue();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

        void CleanupSwapchainResources();

        bool RecreateSwapchain();

        bool m_FramebufferResized = false;

        int m_ImageScaleMode = 1;

        GLFWwindow *m_Window = nullptr;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_SwapchainExtent{};

        VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence m_InFlightFence = VK_NULL_HANDLE;

        VkImage m_TextureImage = VK_NULL_HANDLE;
        VkDeviceMemory m_TextureImageMemory = VK_NULL_HANDLE;
        VkImageView m_TextureImageView = VK_NULL_HANDLE;
        VkSampler m_TextureSampler = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_TextureDescriptorSet = VK_NULL_HANDLE;

        uint32_t m_TextureWidth = 0;
        uint32_t m_TextureHeight = 0;

        uint32_t m_SwapchainImageCount = 0;
        std::unique_ptr<VkImage[]> m_SwapchainImages;
        std::unique_ptr<VkImageView[]> m_SwapchainImageViews;
        std::unique_ptr<VkFramebuffer[]> m_SwapchainFramebuffers;
        std::unique_ptr<VkCommandBuffer[]> m_CommandBuffers;

        VkPipelineLayout m_Rect2DPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Rect2DPipeline = VK_NULL_HANDLE;

        VkBuffer m_Rect2DVertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Rect2DVertexBufferMemory = VK_NULL_HANDLE;
        VkDeviceSize m_Rect2DVertexBufferSize = 0;
        uint32_t m_Rect2DVertexCount = 0;

        const Draw2D *m_Draw2D = nullptr;

        VkDescriptorSetLayout m_SpriteDescriptorSetLayout = VK_NULL_HANDLE;

        VkPipelineLayout m_Sprite2DPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Sprite2DPipeline = VK_NULL_HANDLE;

        VkBuffer m_Sprite2DVertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Sprite2DVertexBufferMemory = VK_NULL_HANDLE;
        VkDeviceSize m_Sprite2DVertexBufferSize = 0;
        uint32_t m_Sprite2DVertexCount = 0;

        std::vector<Draw2DLayerBatch> m_Draw2DLayerBatches;
        std::vector<Sprite2DDrawCall> m_Sprite2DDrawCalls;

        VkClearColorValue m_ClearColor =
        {
            {0.05f, 0.08f, 0.16f, 1.0f}
        };

        void DestroyTextureResources();

        uint32_t FindMemoryType(
            uint32_t typeFilter,
            VkMemoryPropertyFlags properties
        ) const;

        bool CreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory
        ) const;

        bool CreateImage(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage &image,
            VkDeviceMemory &imageMemory
        ) const;

        bool CreateImageView(
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags,
            VkImageView &imageView
        ) const;

        bool CreateDescriptorSetLayout();

        bool CreateGraphicsPipeline();

        bool CreateDescriptorPool();

        bool CreateDescriptorSet();

        bool UpdateTextureDescriptor();

        VkCommandBuffer BeginSingleTimeCommands();

        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        bool TransitionImageLayout(
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout
        );

        bool CopyBufferToImage(
            VkBuffer buffer,
            VkImage image,
            uint32_t width,
            uint32_t height
        );

        bool CreateTextureSampler();
    };
}
