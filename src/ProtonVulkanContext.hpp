#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "ProtonImageLoader.hpp"

namespace Proton
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;

        bool IsComplete() const
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities{};
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanContext
    {
    public:
        VulkanContext() = default;
        ~VulkanContext();

        bool Init(GLFWwindow* window);
        void DrawFrame();
        void NotifyFramebufferResized();
        void Shutdown();

        void SetClearColor(float r, float g, float b, float a);

        bool SetTextureFromImage(const ImageData& image);
        void SetImageScaleMode(int scaleMode);

    private:
        bool CreateInstance();
        bool CreateSurface(GLFWwindow* window);
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

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) ;
        static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) ;
        static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) ;

        void CleanupSwapchainResources();
        bool RecreateSwapchain();
        bool m_FramebufferResized = false;

        int m_ImageScaleMode = 1;

        GLFWwindow* m_Window = nullptr;

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

        VkClearColorValue m_ClearColor =
        {
            { 0.05f, 0.08f, 0.16f, 1.0f }
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
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory
        ) const;

        bool CreateImage(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& imageMemory
        ) const;

        bool CreateImageView(
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags,
            VkImageView& imageView
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