#ifndef __VULKANDESCRIPTOR_H_
#define __VULKANDESCRIPTOR_H_

#include <string.h>
#include <vulkan/vulkan.h>
#include <Graphics/Utils/Rect.h>

namespace Graphics::Backends {
    struct VulkanDescriptor {
        uint32_t        Id;

        VkDescriptorSet VkId;
        Rect            Size;
        int             Channels;

        VkImageView     ImageView;
        VkImage         Image;
        VkDeviceMemory  ImageMemory;
        VkSampler       Sampler;

        VkBuffer		UploadBuffer;
	    VkDeviceMemory	UploadBufferMemory;

        VulkanDescriptor() {
            memset(this, 0, sizeof(VulkanDescriptor));
        }
    };
}

#endif