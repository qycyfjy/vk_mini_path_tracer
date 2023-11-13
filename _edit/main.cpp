// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <cstdio>

#include <nvvk/context_vk.hpp>
#include <nvvk/error_vk.hpp> // NVVK_CHECK macro
#include <nvvk/resourceallocator_vk.hpp>  // NVVK memeory allocators, vulkan低级到没提供malloc
#include <nvvk/structs_vk.hpp>  // vulkan使用sType成员变量来识别void*指向什么结构, 用nvvk::make简化这部分的初始化

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

static const uint64_t RENDER_WIDTH = 800;
static const uint64_t RENDER_HEIGHT = 600;

int main(int argc, const char** argv) {
    nvvk::ContextCreateInfo deviceInfo;
    deviceInfo.apiMajor = 1;
    deviceInfo.apiMinor = 3;
    deviceInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    auto acceleration_structure_feature = nvvk::make<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();
    deviceInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &acceleration_structure_feature);
    auto ray_query_feature = nvvk::make<VkPhysicalDeviceRayQueryFeaturesKHR>();
    deviceInfo.addDeviceExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, false, &ray_query_feature);

    nvvk::Context context;
    context.init(deviceInfo);

    // dedicated会为每个资源进行一次内存分配, 还有nvvk::ResourceAllocatorDma, nvvk::ResourceAllocatorVma这两种分配器
    nvvk::ResourceAllocatorDedicated allocator;
    allocator.init(context, context.m_physicalDevice);

    auto buffer_size_in_bytes = RENDER_WIDTH * RENDER_HEIGHT * 3 * sizeof(float);
    auto buffer_create_info = nvvk::make<VkBufferCreateInfo>();
    buffer_create_info.size = buffer_size_in_bytes;
    // VK_BUFFER_USAGE_STORAGE_BUFFER_BIT表示GPU可读写, VK_BUFFER_USAGE_TRANSFER_DST_BIT表示可作为传输操作的目的地
    buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    auto buffer = allocator.createBuffer(buffer_create_info,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // CPU可读
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | // CPU可缓存
                                         VK_MEMORY_PROPERTY_HOST_CACHED_BIT); // CPU自动缓存管理

    auto cmd_pool_create_info = nvvk::make<VkCommandPoolCreateInfo>();
    cmd_pool_create_info.queueFamilyIndex = context.m_queueGCT;
    VkCommandPool cmd_pool;
    // nullptr指使用默认的Vulkan CPU-side内存分配器
    NVVK_CHECK(vkCreateCommandPool(context, &cmd_pool_create_info, nullptr, &cmd_pool));

    auto cmd_buffer_allocate_info = nvvk::make<VkCommandBufferAllocateInfo>();
    cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_allocate_info.commandPool = cmd_pool;
    cmd_buffer_allocate_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buffer;
    NVVK_CHECK(vkAllocateCommandBuffers(context, &cmd_buffer_allocate_info, &cmd_buffer));

    // begin record command
    auto cmd_buffer_begin_info = nvvk::make<VkCommandBufferBeginInfo>();
    cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // 一次录制只会使用command buffer一次
    NVVK_CHECK(vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info));

    const float fill_value = 0.5f;
    const uint32_t&fill_value_uint32 = reinterpret_cast<const uint32_t &>(fill_value);
    vkCmdFillBuffer(cmd_buffer, buffer.buffer, 0, buffer_size_in_bytes, fill_value_uint32);

    // 让CPU(host)读数据的操作等待transter(如vkCmdFillBuffer)完成
    auto memory_barrier = nvvk::make<VkMemoryBarrier>();
    memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         1, &memory_barrier,
                         0, nullptr, 0, nullptr);

    NVVK_CHECK(vkEndCommandBuffer(cmd_buffer));

    auto submit_info = nvvk::make<VkSubmitInfo>();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;
    NVVK_CHECK(vkQueueSubmit(context.m_queueGCT, 1, &submit_info, VK_NULL_HANDLE));

    // 让CPU等待GCT queue完成
    NVVK_CHECK(vkQueueWaitIdle(context.m_queueGCT));

    // GPU -> CPU
    void* data = allocator.map(buffer);
    stbi_write_hdr("out.hdr", RENDER_WIDTH, RENDER_HEIGHT, 3, static_cast<float *>(data));
    allocator.unmap(buffer);

    vkFreeCommandBuffers(context, cmd_pool, 1, &cmd_buffer);
    vkDestroyCommandPool(context, cmd_pool, nullptr);
    allocator.destroy(buffer);
    allocator.deinit();
    context.deinit();
}
