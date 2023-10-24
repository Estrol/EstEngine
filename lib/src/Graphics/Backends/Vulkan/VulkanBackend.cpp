#include "VulkanBackend.h"
#include <Exceptions/EstException.h>
#include <Graphics/NativeWindow.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <vector>

#include "VulkanDescriptor.h"
#include "vkinit.h"

#include "../../Shaders/image.spv.h"
#include "../../Shaders/position.spv.h"
#include "../../Shaders/solid.spv.h"

using namespace Graphics::Backends;

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

void Vulkan::Init()
{
    CreateInstance();
    InitSwapchain();
    CreateRenderpass();
    InitFramebuffers();
    InitCommands();
    InitSyncStructures();
    InitDescriptors();
    InitShaders();
    InitPipeline();

    m_Initialized = true;
    m_FrameBegin = false;
}

void Vulkan::ReInit()
{
    vkDeviceWaitIdle(m_Vulkan.vkbDevice.device);

    m_PerFrameDeletionQueue.flush();
    m_SwapchainDeletionQueue.flush();

    try {
        bool result = InitSwapchain();
        if (!result)
            return;

        InitFramebuffers();
        InitCommands();
        InitPipeline();
        InitSyncStructures();
    } catch (const Exceptions::EstException &) {
        m_SwapchainReady = false;
    }
}

void Graphics::Backends::Vulkan::Shutdown()
{
    if (m_Initialized) {
        vkDeviceWaitIdle(m_Vulkan.vkbDevice.device);

        if (m_FrameBegin) {
            __debugbreak();
        }

        for (auto &descriptor : m_Descriptors) {
            DestroyDescriptor(descriptor.get(), false);
        }

        m_PerFrameDeletionQueue.flush();
        m_SwapchainDeletionQueue.flush();
        m_Descriptors.clear();
        m_DeletionQueue.flush();

        vkb::destroy_swapchain(m_Swapchain.swapchain);
        vkDestroySurfaceKHR(m_Vulkan.vkbInstance.instance, m_Vulkan.surface, nullptr);

        vkb::destroy_device(m_Vulkan.vkbDevice);
        vkb::destroy_instance(m_Vulkan.vkbInstance);
    }
}

/* Vulkan Create Instance */
void Vulkan::CreateInstance()
{
    memset(&m_Vulkan, 0, sizeof(m_Vulkan));

    if (volkInitialize() != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to initialize Vulkan");
    }

    vkb::InstanceBuilder instance_builder;
    auto instance_builder_return = instance_builder
                                       .set_app_name("EstEngineLibrary")
                                       .set_engine_name("EstEngine")
                                       .request_validation_layers(true)
                                       .require_api_version(1, 1, 0)
                                       .use_default_debug_messenger()
                                       .build();

    if (!instance_builder_return) {
        throw Exceptions::EstException("Failed to create Vulkan instance");
    }

    vkb::Instance vkb_instance = instance_builder_return.value();

    volkLoadInstance(vkb_instance.instance);

    if (!SDL_Vulkan_CreateSurface(
            Graphics::NativeWindow::Get()->GetWindow(),
            vkb_instance.instance,
            &m_Vulkan.surface)) {
        throw Exceptions::EstException("Failed to create Vulkan surface");
    }

    vkb::PhysicalDeviceSelector selector{ vkb_instance };
    vkb::PhysicalDevice physical_device = selector
                                              .set_minimum_version(1, 0)
                                              .set_surface(m_Vulkan.surface)
                                              .select()
                                              .value();

    vkb::DeviceBuilder device_builder{ physical_device };
    vkb::Device vkb_device = device_builder.build().value();

    volkLoadDevice(vkb_device.device);

    auto queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto queueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    m_Vulkan.graphicsQueue = queue;
    m_Vulkan.graphicsQueueFamily = queueFamily;
    m_Vulkan.vkbInstance = vkb_instance;
    m_Vulkan.vkbDevice = vkb_device;
}

bool Vulkan::InitSwapchain()
{
    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();

    vkb::SwapchainBuilder builder{ m_Vulkan.vkbDevice };
    builder.set_desired_format({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_old_swapchain(m_Swapchain.swapchain)
        .set_desired_extent((uint32_t)rect.Width, (uint32_t)rect.Height);

    auto resultbuild = builder.build();
    if (!resultbuild) {
        return false;
    }

    m_Swapchain.swapchain.swapchain = VK_NULL_HANDLE;
    vkb::destroy_swapchain(m_Swapchain.swapchain);
    m_Swapchain.swapchain = resultbuild.value();

    auto vkimages = m_Swapchain.swapchain.get_images();
    if (!vkimages) {
        throw Exceptions::EstException("Failed to get swapchain images");
    }

    auto vkbviews = m_Swapchain.swapchain.get_image_views();
    if (!vkbviews) {
        throw Exceptions::EstException("Failed to get swapchain image views");
    }

    m_Swapchain.imageViews = vkbviews.value();
    m_Swapchain.images = vkimages.value();

    m_Vulkan.depthFormat = VK_FORMAT_D32_SFLOAT;
    m_Vulkan.swapchainFormat = m_Swapchain.swapchain.image_format;

    VkExtent3D depthImageExtent = {
        (uint32_t)rect.Width,
        (uint32_t)rect.Height,
        1
    };

    VkImageCreateInfo dimg_info = vkinit::image_create_info(m_Vulkan.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    auto result = vkCreateImage(m_Vulkan.vkbDevice.device, &dimg_info, nullptr, &m_Swapchain.depthImage);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to allocate depth image memory");
    }

    // allocate memory
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Vulkan.vkbDevice.device, m_Swapchain.depthImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkinit::find_memory_type(m_Vulkan.vkbDevice.physical_device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(m_Vulkan.vkbDevice.device, &allocInfo, nullptr, &m_Swapchain.depthImageMemory);
        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to allocate image memory");
        }

        vkBindImageMemory(m_Vulkan.vkbDevice.device, m_Swapchain.depthImage, m_Swapchain.depthImageMemory, 0);
    }

    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(m_Vulkan.depthFormat, m_Swapchain.depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);
    result = vkCreateImageView(m_Vulkan.vkbDevice.device, &dview_info, nullptr, &m_Swapchain.depthImageView);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to allocate depth image memory");
    }

    m_SwapchainDeletionQueue.push_function([=] {
        vkDestroyImageView(m_Vulkan.vkbDevice.device, m_Swapchain.depthImageView, nullptr);
        vkDestroyImage(m_Vulkan.vkbDevice.device, m_Swapchain.depthImage, nullptr);
        vkFreeMemory(m_Vulkan.vkbDevice.device, m_Swapchain.depthImageMemory, nullptr);

        m_Swapchain.depthImageView = VK_NULL_HANDLE;
        m_Swapchain.depthImage = VK_NULL_HANDLE;
        m_Swapchain.depthImageMemory = VK_NULL_HANDLE;
    });

    m_SwapchainReady = true;
    return true;
}

void Vulkan::CreateRenderpass()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = m_Vulkan.swapchainFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.flags = 0;
    depth_attachment.format = m_Vulkan.depthFormat;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depth_dependency = {};
    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = &dependencies[0];

    auto result = vkCreateRenderPass(m_Vulkan.vkbDevice.device, &render_pass_info, nullptr, &m_Swapchain.renderpass);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create render pass");
    }

    m_DeletionQueue.push_function([=]() { vkDestroyRenderPass(m_Vulkan.vkbDevice.device, m_Swapchain.renderpass, nullptr); });
}

void Vulkan::InitFramebuffers()
{
    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();
    VkExtent2D _windowExtent = { (uint32_t)rect.Width, (uint32_t)rect.Height };
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(m_Swapchain.renderpass, _windowExtent);

    const uint32_t swapchain_imagecount = (uint32_t)m_Swapchain.images.size();
    m_Swapchain.framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

    for (uint32_t i = 0; i < swapchain_imagecount; i++) {
        VkImageView attachments[2];
        attachments[0] = m_Swapchain.imageViews[i];
        attachments[1] = m_Swapchain.depthImageView;

        fb_info.pAttachments = attachments;
        fb_info.attachmentCount = 2;

        auto result = vkCreateFramebuffer(m_Vulkan.vkbDevice.device, &fb_info, nullptr, &m_Swapchain.framebuffers[i]);
        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create framebuffer");
        }

        m_SwapchainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(m_Vulkan.vkbDevice.device, m_Swapchain.framebuffers[i], nullptr);
            vkDestroyImageView(m_Vulkan.vkbDevice.device, m_Swapchain.imageViews[i], nullptr);

            m_Swapchain.framebuffers[i] = VK_NULL_HANDLE;
            m_Swapchain.imageViews[i] = VK_NULL_HANDLE;
        });
    }
}

void Vulkan::InitCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(m_Vulkan.graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    m_Swapchain.frames.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto result = vkCreateCommandPool(m_Vulkan.vkbDevice.device, &commandPoolInfo, nullptr, &m_Swapchain.frames[i].commandPool);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create command pool");
        }

        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_Swapchain.frames[i].commandPool, 1);

        result = vkAllocateCommandBuffers(m_Vulkan.vkbDevice.device, &cmdAllocInfo, &m_Swapchain.frames[i].commandBuffer);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to allocate command buffer");
        }

        m_Swapchain.frames[i].isValid = true;
        m_SwapchainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(m_Vulkan.vkbDevice.device, m_Swapchain.frames[i].commandPool, nullptr);
            m_Swapchain.frames[i].commandPool = VK_NULL_HANDLE; });
    }

    VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(m_Vulkan.graphicsQueueFamily);
    auto result = vkCreateCommandPool(m_Vulkan.vkbDevice.device, &uploadCommandPoolInfo, nullptr, &m_Swapchain.uploadContext.commandPool);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create upload command pool");
    }

    m_SwapchainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(m_Vulkan.vkbDevice.device, m_Swapchain.uploadContext.commandPool, nullptr);
    });

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_Swapchain.uploadContext.commandPool, 1);

    result = vkAllocateCommandBuffers(m_Vulkan.vkbDevice.device, &cmdAllocInfo, &m_Swapchain.uploadContext.commandBuffer);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to allocate upload command buffer");
    }

    constexpr uint32_t MAX_VERTEX_OBJECTS = 50000;
    constexpr uint32_t MAX_VERTEX_BUFFER_SIZE = sizeof(Vertex) * MAX_VERTEX_OBJECTS;
    constexpr uint32_t MAX_INDEX_BUFFER_SIZE = sizeof(uint32_t) * MAX_VERTEX_OBJECTS;

    memset(&m_Swapchain.vertexBuffer, 0, sizeof(m_Swapchain.vertexBuffer));
    memset(&m_Swapchain.indexBuffer, 0, sizeof(m_Swapchain.indexBuffer));

    ResizeBuffer(MAX_VERTEX_BUFFER_SIZE, MAX_INDEX_BUFFER_SIZE);

    m_Swapchain.maxVertexBufferSize = MAX_VERTEX_BUFFER_SIZE;
    m_Swapchain.maxIndexBufferSize = MAX_INDEX_BUFFER_SIZE;

    m_SwapchainDeletionQueue.push_function([=] {
        vkDestroyBuffer(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.buffer, nullptr);
        vkFreeMemory(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.memory, nullptr);

        vkDestroyBuffer(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.buffer, nullptr);
        vkFreeMemory(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.memory, nullptr); });
}

void Vulkan::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto result = vkCreateFence(m_Vulkan.vkbDevice.device, &fenceCreateInfo, nullptr, &m_Swapchain.frames[i].renderFence);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create fence");
        }

        m_SwapchainDeletionQueue.push_function([=]() { vkDestroyFence(m_Vulkan.vkbDevice.device, m_Swapchain.frames[i].renderFence, nullptr); });

        result = vkCreateSemaphore(m_Vulkan.vkbDevice.device, &semaphoreCreateInfo, nullptr, &m_Swapchain.frames[i].presentSemaphore);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create preset fence");
        }

        result = vkCreateSemaphore(m_Vulkan.vkbDevice.device, &semaphoreCreateInfo, nullptr, &m_Swapchain.frames[i].renderSemaphore);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create render fence");
        }

        m_SwapchainDeletionQueue.push_function([=]() {
            vkDestroySemaphore(m_Vulkan.vkbDevice.device, m_Swapchain.frames[i].presentSemaphore, nullptr);
            vkDestroySemaphore(m_Vulkan.vkbDevice.device, m_Swapchain.frames[i].renderSemaphore, nullptr); });
    }

    auto result = vkCreateFence(m_Vulkan.vkbDevice.device, &fenceCreateInfo, nullptr, &m_Swapchain.uploadContext.renderFence);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create upload fence");
    }

    m_SwapchainDeletionQueue.push_function([=]() { vkDestroyFence(m_Vulkan.vkbDevice.device, m_Swapchain.uploadContext.renderFence, nullptr); });
}

void Vulkan::InitDescriptors()
{
    std::vector<VkDescriptorPoolSize> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 10;
    poolInfo.poolSizeCount = (uint32_t)sizes.size();
    poolInfo.pPoolSizes = sizes.data();

    auto result = vkCreateDescriptorPool(m_Vulkan.vkbDevice.device, &poolInfo, nullptr, &m_Vulkan.descriptorPool);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create descriptor pool");
    }

    VkDescriptorSetLayoutBinding binding[1] = {};
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = binding;

    result = vkCreateDescriptorSetLayout(m_Vulkan.vkbDevice.device, &info, nullptr, &m_Vulkan.descriptorSetLayout);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create descriptor set layout");
    }

    m_DeletionQueue.push_function([=] {
        vkDestroyDescriptorSetLayout(m_Vulkan.vkbDevice.device, m_Vulkan.descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(m_Vulkan.vkbDevice.device, m_Vulkan.descriptorPool, nullptr);
    });
}

void Vulkan::InitShaders()
{
    const uint32_t *vertShaderCode = __glsl_position;
    const uint32_t *solidFragShaderCode = __glsl_solid;
    const uint32_t *imageFragShaderCode = __glsl_image;

    size_t vertShaderSize = sizeof(__glsl_position);
    size_t solidFragShaderSize = sizeof(__glsl_solid);
    size_t imageFragShaderSize = sizeof(__glsl_image);

    VkShaderModuleCreateInfo vertShaderInfo = {};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderInfo.codeSize = vertShaderSize;
    vertShaderInfo.pCode = vertShaderCode;

    VkShaderModuleCreateInfo solidFragShaderInfo = {};
    solidFragShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    solidFragShaderInfo.codeSize = solidFragShaderSize;
    solidFragShaderInfo.pCode = solidFragShaderCode;

    VkShaderModuleCreateInfo imageFragShaderInfo = {};
    imageFragShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    imageFragShaderInfo.codeSize = imageFragShaderSize;
    imageFragShaderInfo.pCode = imageFragShaderCode;

    auto result = vkCreateShaderModule(m_Vulkan.vkbDevice.device, &vertShaderInfo, nullptr, &m_Vulkan.vertShaderModule);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create vertex shader module");
    }

    result = vkCreateShaderModule(m_Vulkan.vkbDevice.device, &solidFragShaderInfo, nullptr, &m_Vulkan.solidFragShaderModule);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create solid fragment shader module");
    }

    result = vkCreateShaderModule(m_Vulkan.vkbDevice.device, &imageFragShaderInfo, nullptr, &m_Vulkan.imageFragShaderModule);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to create image fragment shader module");
    }

    m_DeletionQueue.push_function([=]() {
        vkDestroyShaderModule(m_Vulkan.vkbDevice.device, m_Vulkan.vertShaderModule, nullptr);
        vkDestroyShaderModule(m_Vulkan.vkbDevice.device, m_Vulkan.solidFragShaderModule, nullptr);
        vkDestroyShaderModule(m_Vulkan.vkbDevice.device, m_Vulkan.imageFragShaderModule, nullptr); });
}

void Vulkan::InitPipeline()
{
    VkResult result;
    VkDescriptorSetLayout image_descriptor_layout;
    {
        VkDescriptorSetLayoutBinding binding[1] = {};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding;

        result = vkCreateDescriptorSetLayout(m_Vulkan.vkbDevice.device, &info, nullptr, &image_descriptor_layout);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create descriptor set layout");
        }
    }

    VkPipelineLayout pipeline_layout;
    {
        VkPushConstantRange push_constants[1] = {};
        push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 4;
        VkDescriptorSetLayout set_layout[1] = { image_descriptor_layout };
        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = push_constants;

        result = vkCreatePipelineLayout(m_Vulkan.vkbDevice.device, &layout_info, nullptr, &pipeline_layout);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create pipeline layout");
        }
    }

    std::map<ShaderFragmentType, std::pair<VkShaderModule, VkShaderModule>> shaders = {
        { ShaderFragmentType::Image, { m_Vulkan.vertShaderModule, m_Vulkan.imageFragShaderModule } },
        { ShaderFragmentType::Solid, { m_Vulkan.vertShaderModule, m_Vulkan.solidFragShaderModule } }
    };

    for (auto &[type, shader_pair] : shaders) {
        VkPipelineShaderStageCreateInfo stage[2] = {};
        stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stage[0].module = shader_pair.first;
        stage[0].pName = "main";
        stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stage[1].module = shader_pair.second;
        stage[1].pName = "main";

        VkVertexInputBindingDescription binding_desc[1] = {};
        binding_desc[0].stride = sizeof(Vertex);
        binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attribute_desc[3] = {};
        attribute_desc[0].location = 0;
        attribute_desc[0].binding = binding_desc[0].binding;
        attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc[0].offset = MY_OFFSETOF(Vertex, pos);
        attribute_desc[1].location = 1;
        attribute_desc[1].binding = binding_desc[0].binding;
        attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc[1].offset = MY_OFFSETOF(Vertex, texCoord);
        attribute_desc[2].location = 2;
        attribute_desc[2].binding = binding_desc[0].binding;
        attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attribute_desc[2].offset = MY_OFFSETOF(Vertex, color);

        VkPipelineVertexInputStateCreateInfo vertex_info = {};
        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_info.vertexBindingDescriptionCount = 1;
        vertex_info.pVertexBindingDescriptions = binding_desc;
        vertex_info.vertexAttributeDescriptionCount = 3;
        vertex_info.pVertexAttributeDescriptions = attribute_desc;

        VkPipelineInputAssemblyStateCreateInfo ia_info = {};
        ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        ia_info.primitiveRestartEnable = VK_TRUE;

        VkPipelineViewportStateCreateInfo viewport_info = {};
        viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_info.viewportCount = 1;
        viewport_info.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster_info = {};
        raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster_info.polygonMode = VK_POLYGON_MODE_FILL;
        raster_info.cullMode = VK_CULL_MODE_NONE;
        raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster_info.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms_info = {};
        ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth_info = {};
        depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendStateCreateInfo blend_info = {};
        blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend_info.attachmentCount = 1;

        VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state = {};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = (uint32_t)(sizeof(dynamic_states) / sizeof(dynamic_states[0]));
        dynamic_state.pDynamicStates = dynamic_states;

        VkPipeline pipeline;
        {
            VkPipelineColorBlendAttachmentState color_attachment[1] = {};
            color_attachment[0].blendEnable = VK_TRUE;
            color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
            color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
            color_attachment[0].colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            blend_info.pAttachments = color_attachment;

            VkGraphicsPipelineCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            info.flags = 0;
            info.stageCount = 2;
            info.pStages = stage;
            info.pVertexInputState = &vertex_info;
            info.pInputAssemblyState = &ia_info;
            info.pViewportState = &viewport_info;
            info.pRasterizationState = &raster_info;
            info.pMultisampleState = &ms_info;
            info.pDepthStencilState = &depth_info;
            info.pColorBlendState = &blend_info;
            info.pDynamicState = &dynamic_state;
            info.layout = pipeline_layout;
            info.renderPass = m_Swapchain.renderpass;
            info.subpass = 0;

            result = vkCreateGraphicsPipelines(m_Vulkan.vkbDevice.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);

            if (result != VK_SUCCESS) {
                throw Exceptions::EstException("Failed to create graphics pipeline");
            }
        }

        m_Swapchain.pipelines[type] = pipeline;

        m_SwapchainDeletionQueue.push_function([=] {
            vkDestroyPipeline(m_Vulkan.vkbDevice.device, pipeline, nullptr);
        });
    }

    m_Swapchain.pipelineLayout = pipeline_layout;
    m_SwapchainDeletionQueue.push_function([=] {
        vkDestroyPipelineLayout(m_Vulkan.vkbDevice.device, m_Swapchain.pipelineLayout, nullptr);
    });

    m_DeletionQueue.push_function([=] {
        vkDestroyDescriptorSetLayout(m_Vulkan.vkbDevice.device, image_descriptor_layout, nullptr);
    });
}

void Vulkan::ImmediateSubmit(std::function<void(VkCommandBuffer)> &&function)
{
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    auto result = vkBeginCommandBuffer(m_Swapchain.uploadContext.commandBuffer, &cmdBeginInfo);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to begin command buffer");
    }

    function(m_Swapchain.uploadContext.commandBuffer);

    result = vkEndCommandBuffer(m_Swapchain.uploadContext.commandBuffer);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to end command buffer");
    }

    VkSubmitInfo submit = vkinit::submit_info(&m_Swapchain.uploadContext.commandBuffer);

    result = vkQueueSubmit(m_Vulkan.graphicsQueue, 1, &submit, VK_NULL_HANDLE);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to submit queue");
    }

    vkWaitForFences(m_Vulkan.vkbDevice.device, 1, &m_Swapchain.uploadContext.renderFence, true, 9999999999);
    vkResetFences(m_Vulkan.vkbDevice.device, 1, &m_Swapchain.uploadContext.renderFence);

    vkResetCommandPool(m_Vulkan.vkbDevice.device, m_Swapchain.uploadContext.commandPool, 0);
}

VulkanFrame &Vulkan::GetCurrentFrame()
{
    return m_Swapchain.frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT];
}

VulkanFrame &Vulkan::GetLastFrame()
{
    return m_Swapchain.frames[(m_CurrentFrame - 1) % MAX_FRAMES_IN_FLIGHT];
}

bool Vulkan::BeginFrame()
{
    if (!m_SwapchainReady) {
        return false;
    }

    auto result = vkDeviceWaitIdle(m_Vulkan.vkbDevice.device);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to wait for device idle");
    }

    auto &frame = GetCurrentFrame();

    result = vkWaitForFences(m_Vulkan.vkbDevice.device, 1, &frame.renderFence, true, 9999999999);
    if (result == VK_TIMEOUT) {
        return false;
    } else if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to wait for fence");
    }

    result = vkResetFences(m_Vulkan.vkbDevice.device, 1, &frame.renderFence);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to reset fence");
    }

    result = vkAcquireNextImageKHR(m_Vulkan.vkbDevice.device, m_Swapchain.swapchain, UINT64_MAX, frame.presentSemaphore, VK_NULL_HANDLE, &m_Swapchain.swapchainIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_SwapchainReady = false;
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw Exceptions::EstException("Failed to acquire next image");
    }

    result = vkResetFences(m_Vulkan.vkbDevice.device, 1, &frame.renderFence);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to reset fence");
    }

    result = vkResetCommandPool(m_Vulkan.vkbDevice.device, frame.commandPool, 0);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to reset command pool");
    }

    m_PerFrameDeletionQueue.flush();

    if (!frame.isValid) {
        return false;
    }

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    result = vkBeginCommandBuffer(frame.commandBuffer, &cmdBeginInfo);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to begin command buffer");
    }

    VkClearValue clearValue;
    clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();
    VkExtent2D _windowExtent = {
        (uint32_t)rect.Width,
        (uint32_t)rect.Height
    };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(m_Swapchain.renderpass, _windowExtent, m_Swapchain.framebuffers[m_Swapchain.swapchainIndex]);

    VkClearValue clearValues[] = { clearValue, depthClear };
    rpInfo.pClearValues = &clearValues[0];
    rpInfo.clearValueCount = 2;

    vkCmdBeginRenderPass(frame.commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_FrameBegin = true;

    return true;
}

void Vulkan::EndFrame()
{
    if (!m_SwapchainReady) {
        throw Exceptions::EstException("Attempt to end-frame on swap chain not ready");
    }

    FlushQueue();

    auto &frame = GetCurrentFrame();
    vkCmdEndRenderPass(frame.commandBuffer);

    auto result = vkEndCommandBuffer(frame.commandBuffer);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to end command buffer");
    }

    VkSubmitInfo submit = vkinit::submit_info(&frame.commandBuffer);
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame.presentSemaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame.renderSemaphore;

    result = vkQueueSubmit(m_Vulkan.graphicsQueue, 1, &submit, frame.renderFence);

    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to submit queue");
    }

    VkPresentInfoKHR presentInfo = vkinit::present_info();
    presentInfo.pSwapchains = &m_Swapchain.swapchain.swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &frame.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &m_Swapchain.swapchainIndex;

    result = vkQueuePresentKHR(m_Vulkan.graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_SwapchainReady = false;
    } else {
        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to submit swap buffer");
        }
    }

    m_CurrentFrame++;
    m_FrameBegin = false;
}

bool Vulkan::NeedReinit()
{
    return !m_SwapchainReady;
}

void Vulkan::Push(SubmitInfo &info)
{
    submitInfos.push_back(info);
}

void Vulkan::FlushQueue()
{
    if (submitInfos.size() <= 0) {
        return;
    }

    if (!m_SwapchainReady) {
        submitInfos.clear();
        return;
    }

    auto &frame = GetCurrentFrame();
    if (!frame.isValid) {
        submitInfos.clear();
        return;
    }

    std::sort(submitInfos.begin(), submitInfos.end(), [](SubmitInfo &a, SubmitInfo &b) {
        return a.zIndex < b.zIndex;
    });

    VkDeviceSize vertex_size = 0;
    VkDeviceSize indices_size = 0;
    for (auto &info : submitInfos) {
        vertex_size += info.vertices.size() * sizeof(info.vertices[0]);
        indices_size += info.indices.size() * sizeof(info.indices[0]);
    }

    if (vertex_size >= m_Swapchain.maxVertexBufferSize || indices_size >= m_Swapchain.maxIndexBufferSize) {
        ResizeBuffer(vertex_size, indices_size);
    }

    void *vertexPtr;
    void *indicePtr;

    auto result = vkMapMemory(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.memory, 0, vertex_size, 0, &vertexPtr);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to map GPU's vertex buffer");
    }

    result = vkMapMemory(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.memory, 0, indices_size, 0, &indicePtr);
    if (result != VK_SUCCESS) {
        throw Exceptions::EstException("Failed to map GPU's index buffer");
    }

    auto rect = Graphics::NativeWindow::Get()->GetWindowSize();
    float scale[2];
    scale[0] = 2.0f / rect.Width;
    scale[1] = 2.0f / rect.Height;

    float translate[2];
    translate[0] = -1.0f;
    translate[1] = -1.0f;

    VkDeviceSize offset = 0;
    for (auto &info : submitInfos) {
        if (offset >= m_Swapchain.maxVertexBufferSize) {
            continue;
        }

        memcpy((char *)vertexPtr + offset, info.vertices.data(), info.vertices.size() * sizeof(info.vertices[0]));
        offset += info.vertices.size() * sizeof(info.vertices[0]);
    }

    offset = 0;
    for (auto &info : submitInfos) {
        memcpy((char *)indicePtr + offset, info.indices.data(), info.indices.size() * sizeof(info.indices[0]));
        offset += info.indices.size() * sizeof(info.indices[0]);
    }

    vkUnmapMemory(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.memory);
    vkUnmapMemory(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.memory);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)rect.Width;
    viewport.height = (float)rect.Height;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, &m_Swapchain.vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(frame.commandBuffer, m_Swapchain.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    int currentVertIndex = 0; // vertex
    int currentIndiIndex = 0; // indicies

    for (auto &info : submitInfos) {
        auto pipeline = m_Swapchain.pipelineLayout;
        auto graphics = m_Swapchain.pipelines[info.fragmentType];

        vkCmdPushConstants(frame.commandBuffer, pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, scale);
        vkCmdPushConstants(frame.commandBuffer, pipeline, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

        VkDescriptorSet image = (VkDescriptorSet)(info.image != 0 ? (void *)info.image : VK_NULL_HANDLE);
        uint32_t imageCount = 1;
        if (image == NULL) {
            imageCount = 0;
        }

        vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline, 0, 1, &image, 0, nullptr);
        vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics);

        VkRect2D clip = {};
        clip.offset = {
            info.clipRect.X, info.clipRect.Y
        };
        clip.extent = {
            (uint32_t)info.clipRect.Width, (uint32_t)info.clipRect.Height
        };

        vkCmdSetScissor(frame.commandBuffer, 0, 1, &clip);

        vkCmdDrawIndexed(frame.commandBuffer, (uint32_t)info.vertices.size(), 1, currentIndiIndex, currentVertIndex, 0);
        currentVertIndex += (int)info.vertices.size();
        currentIndiIndex += (int)info.indices.size();
    }

    submitInfos.clear();
}

void Vulkan::ResizeBuffer(VkDeviceSize vertices, VkDeviceSize indicies)
{
    auto result = VK_SUCCESS;

    if (vertices > m_Swapchain.maxVertexBufferSize) {
        m_Swapchain.maxVertexBufferSize = (uint32_t)vertices;
    }

    if (indicies > m_Swapchain.maxIndexBufferSize) {
        m_Swapchain.maxIndexBufferSize = (uint32_t)indicies;
    }

    if (m_Swapchain.vertexBuffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.buffer, nullptr);
        vkFreeMemory(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.memory, nullptr);
    }

    if (m_Swapchain.indexBuffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.buffer, nullptr);
        vkFreeMemory(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.memory, nullptr);
    }

    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = vertices;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        result = vkCreateBuffer(m_Vulkan.vkbDevice.device, &bufferInfo, nullptr, &m_Swapchain.vertexBuffer.buffer);
        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create vertex buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkinit::find_memory_type(
            m_Vulkan.vkbDevice.physical_device,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(m_Vulkan.vkbDevice.device, &allocInfo, nullptr, &m_Swapchain.vertexBuffer.memory);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to allocate vertex buffer memory");
        }

        vkBindBufferMemory(m_Vulkan.vkbDevice.device, m_Swapchain.vertexBuffer.buffer, m_Swapchain.vertexBuffer.memory, 0);
    }

    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = indicies;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        result = vkCreateBuffer(m_Vulkan.vkbDevice.device, &bufferInfo, nullptr, &m_Swapchain.indexBuffer.buffer);
        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to create index buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkinit::find_memory_type(
            m_Vulkan.vkbDevice.physical_device,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(m_Vulkan.vkbDevice.device, &allocInfo, nullptr, &m_Swapchain.indexBuffer.memory);

        if (result != VK_SUCCESS) {
            throw Exceptions::EstException("Failed to allocate index buffer memory");
        }

        vkBindBufferMemory(m_Vulkan.vkbDevice.device, m_Swapchain.indexBuffer.buffer, m_Swapchain.indexBuffer.memory, 0);
    }
}

VulkanDescriptor *Vulkan::CreateDescriptor()
{
    auto descriptor = std::make_unique<VulkanDescriptor>();
    descriptor->Id = ++m_DescriptorId;

    m_Descriptors.push_back(std::move(descriptor));
    return m_Descriptors.back().get();
}

void Vulkan::DestroyDescriptor(VulkanDescriptor *descriptor, bool _delete)
{
    auto device = m_Vulkan.vkbDevice.device;
    auto descriptorPool = m_Vulkan.descriptorPool;
    auto uploadBufferMemory = descriptor->UploadBufferMemory;
    auto uploadBuffer = descriptor->UploadBuffer;
    auto sampler = descriptor->Sampler;
    auto imageView = descriptor->ImageView;
    auto image = descriptor->Image;
    auto imageMemory = descriptor->ImageMemory;
    auto vkId = descriptor->VkId;

    m_PerFrameDeletionQueue.push_function([=] {
        vkFreeMemory(device, uploadBufferMemory, nullptr);
        vkDestroyBuffer(device, uploadBuffer, nullptr);
        vkDestroySampler(device, sampler, nullptr);
        vkDestroyImageView(device, imageView, nullptr);
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, imageMemory, nullptr);
        vkFreeDescriptorSets(device, descriptorPool, 1, &vkId);
    });

    auto it = std::find_if(m_Descriptors.begin(), m_Descriptors.end(), [descriptor](auto &item) {
        return item.get()->Id == descriptor->Id;
    });

    if (_delete) {
        if (it != m_Descriptors.end()) {
            m_Descriptors.erase(it);
        }
    }
}

VulkanObject *Vulkan::GetVulkanObject()
{
    return &m_Vulkan;
}

VulkanSwapChain *Vulkan::GetSwapchain()
{
    return &m_Swapchain;
}

void Vulkan::ImGui_Init()
{
}

void Vulkan::ImGui_DeInit()
{
}

void Vulkan::ImGui_NewFrame()
{
}

void Vulkan::ImGui_EndFrame()
{
}
