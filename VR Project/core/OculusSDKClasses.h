#pragma once

#include "Common.h"
#include <OVR_CAPI_Vk.h>
#include <Extras/OVR_Math.h>
using namespace OVR;
// Base class for object wrappers which manage lifetime with Create/Release
class VulkanObject
{
public:
    VkResult lastResult = VK_SUCCESS;
    virtual void Release() {};
    ~VulkanObject()
    {
        Release();
    }
};


// DepthBuffer
class DepthBuffer: public VulkanObject
{
public:
    VkFormat                format;
    VkDeviceMemory          mem;
    VkImage                 img;
    VkImageView             view;
	VkDevice				device;

    DepthBuffer() :
        format(VK_FORMAT_D32_SFLOAT),
        mem(),
        img(),
        view(),
		device()

    {
    }

    bool Create(VkExtent2D extent, VkDevice logicdevice, VkPhysicalDevice physicalDevice, VkFormat aFormat = VK_FORMAT_D32_SFLOAT)
    {
        format = aFormat;
		device = logicdevice;

        VkImageCreateInfo imgInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.format = format;
        imgInfo.extent = { extent.width, extent.height, 1 };
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        vkCreateImage(device, &imgInfo, nullptr, &img);

        //Platform.AllocateImageMemory(img, &mem);
        VkMemoryRequirements memReqs = {};
        vkGetImageMemoryRequirements(device, img, &memReqs);
        // Search memtypes to find first index with those properties
		VkPhysicalDeviceMemoryProperties memProps = {};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if (memReqs.memoryTypeBits & (1 << i))
            {
                // Type is available, does it match user properties?
                if ((memProps.memoryTypes[i].propertyFlags & 0) == 0)
                {
                    VkMemoryAllocateInfo memAlloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
                    memAlloc.allocationSize = memReqs.size;
                    memAlloc.memoryTypeIndex = i;
                    vkAllocateMemory(device, &memAlloc, nullptr, &mem);
					break;
                }
            }
        }


        vkBindImageMemory(device, img, mem, 0);

        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = img;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.flags = 0;
        vkCreateImageView(device, &viewInfo, nullptr, &view);

        return true;
    }

    void Release()
    {
        if (device)
        {
            if (view) vkDestroyImageView(device, view, nullptr);
            if (img) vkDestroyImage(device, img, nullptr);
            if (mem) vkFreeMemory(device, mem, nullptr);
        }
        view = VK_NULL_HANDLE;
        img = VK_NULL_HANDLE;
        mem = VK_NULL_HANDLE;
    }
};


// RenderPass wrapper
class RenderPass: public VulkanObject
{
public:
    VkRenderPass            pass;
	VkDevice				device;

    RenderPass() :
        pass(VK_NULL_HANDLE),
		device(VK_NULL_HANDLE)
    {
    }

    bool RenderPass::Create(VkFormat colorFmt, VkFormat depthFmt, VkDevice logicdevice)
    {
		device = logicdevice;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        VkAttachmentReference depthRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        std::array<VkAttachmentDescription, 2> at = {};

        VkRenderPassCreateInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rpInfo.attachmentCount = 0;
        rpInfo.pAttachments = at.data();
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;

        if (colorFmt != VK_FORMAT_UNDEFINED)
        {
            colorRef.attachment = rpInfo.attachmentCount++;

            at[colorRef.attachment].format = colorFmt;
            at[colorRef.attachment].samples = VK_SAMPLE_COUNT_1_BIT;
            at[colorRef.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            at[colorRef.attachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            at[colorRef.attachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            at[colorRef.attachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[colorRef.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            at[colorRef.attachment].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorRef;
        }

        if (depthFmt != VK_FORMAT_UNDEFINED)
        {
            depthRef.attachment = rpInfo.attachmentCount++;

            at[depthRef.attachment].format = depthFmt;
            at[depthRef.attachment].samples = VK_SAMPLE_COUNT_1_BIT;
            at[depthRef.attachment].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            at[depthRef.attachment].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[depthRef.attachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            at[depthRef.attachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[depthRef.attachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            at[depthRef.attachment].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpass.pDepthStencilAttachment = &depthRef;
        }

        (vkCreateRenderPass(device, &rpInfo, nullptr, &pass));

        return true;
    }

    void Release()
    {
        if (device)
        {
            if (pass) vkDestroyRenderPass(device, pass, nullptr);
        }
        pass = VK_NULL_HANDLE;
    }
};

// Framebuffer wrapper
class Framebuffer: public VulkanObject
{
public:
    VkFramebuffer   fb;
	VkDevice		device;

    Framebuffer() :
        fb(VK_NULL_HANDLE), 
		device(VK_NULL_HANDLE)
    {
    }

    bool Create(VkExtent2D extent, const RenderPass& rp, VkImageView colorView, VkImageView depthView, VkDevice logicdevice)
    {
		device = logicdevice;
        std::array<VkImageView, 2> attachments = {};
        uint32_t attachmentCount = 0;
        if (colorView != VK_NULL_HANDLE)
            attachments[attachmentCount++] = colorView;
        if (depthView != VK_NULL_HANDLE)
            attachments[attachmentCount++] = depthView;

        VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO  };
        fbInfo.renderPass = rp.pass;
        fbInfo.attachmentCount = attachmentCount;
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;
        (vkCreateFramebuffer(device, &fbInfo, nullptr, &fb));

        return true;
    }

    void Release()
    {
        if (device)
        {
            if (fb) vkDestroyFramebuffer(device, fb, nullptr);
        }
        fb = VK_NULL_HANDLE;
    }
};


// VkImage + framebuffer wrapper
class RenderTexture: public VulkanObject
{
public:
    VkImage         image;
    VkImageView     view;
    Framebuffer     fb;
	VkDevice		device;

    RenderTexture() :
        image(VK_NULL_HANDLE),
        view(VK_NULL_HANDLE),
        fb(),
		device(VK_NULL_HANDLE)
    {
    }

    bool Create(VkImage anImage, VkFormat format, VkExtent2D size, RenderPass& renderPass, VkImageView depthView, VkDevice logicdevice)
    {
		device = logicdevice;
        image = anImage;

        // Create image view
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &view);

		fb.Create(size, renderPass, view, depthView,device);

        return true;
    }

    void Release()
    {
        fb.Release();
        if (view) vkDestroyImageView(device, view, nullptr);
        // Note we don't own image, it will get destroyed when ovr_DestroyTextureSwapChain is called
        image = VK_NULL_HANDLE;
        view = VK_NULL_HANDLE;
    }
};

// ovrSwapTextureSet wrapper class for Vulkan rendering
class TextureSwapChain: public VulkanObject
{
public:
    ovrSession                  session;
    VkExtent2D                  size;
    ovrTextureSwapChain         textureChain;
    std::vector<RenderTexture>  texElements;
	VkDevice					device;

    TextureSwapChain() :
        session(nullptr),
        size{},
        textureChain(nullptr),
        texElements()
    {
    }

    bool Create(ovrSession aSession, VkExtent2D aSize, RenderPass& renderPass, VkImageView depthBufferView, VkDevice logicdevice)
    {
		device = logicdevice;
        session = aSession;
        size = aSize;

        ovrTextureSwapChainDesc desc = {};
        desc.Type = ovrTexture_2D;
        desc.ArraySize = 1;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.Width = (int)size.width;
        desc.Height = (int)size.height;
        desc.MipLevels = 1;
        desc.SampleCount = 1;
        desc.MiscFlags = ovrTextureMisc_DX_Typeless;
        desc.BindFlags = ovrTextureBind_DX_RenderTarget;
        desc.StaticImage = ovrFalse;

        ovrResult result = ovr_CreateTextureSwapChainVk(session, device, &desc, &textureChain);
        if (!OVR_SUCCESS(result))
            return false;

        int textureCount = 0;
        ovr_GetTextureSwapChainLength(session, textureChain, &textureCount);
        //texElements.reserve(textureCount);
        texElements.resize(textureCount);
        for (int i = 0; i < textureCount; ++i)
        {
            VkImage image;
            result = ovr_GetTextureSwapChainBufferVk(session, textureChain, i, &image);
            //texElements.emplace_back(RenderTexture());
            //texElements.back().Create(image, VK_FORMAT_R8G8B8A8_SRGB, size, renderPass, depthBufferView, device);
			//texElements[i] = RenderTexture();
            texElements[i].Create(image, VK_FORMAT_R8G8B8A8_SRGB, size, renderPass, depthBufferView, device);
        }

        return true;
    }

    const Framebuffer& GetFramebuffer()
    {
        int index = 0;
        ovr_GetTextureSwapChainCurrentIndex(session, textureChain, &index);
        return texElements[index].fb;
    }

    OVR::Recti GetViewport()
    {
        return OVR::Recti(0, 0, size.width, size.height);
    }

    // Commit changes
    void Commit()
    {
        ovr_CommitTextureSwapChain(session, textureChain);
    }

    void Release()
    {
        if (device)
        {
            for (auto& te: texElements)
            {
                te.Release();
            }
        }
        if (session && textureChain)
        {
            ovr_DestroyTextureSwapChain(session, textureChain);
        }
        texElements.clear();
        textureChain = nullptr;
        session = nullptr;
    }
};