// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <cstdio>

#include <nvvk/context_vk.hpp>
#include <nvvk/resourceallocator_vk.hpp>  // NVVK memeory allocators, vulkan低级到没提供malloc
#include <nvvk/structs_vk.hpp>  // vulkan使用sType成员变量来识别void*指向什么结构, 用nvvk::make简化这部分的初始化

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

    // GPU -> CPU
    void* data = allocator.map(buffer);
    float* image = static_cast<float *>(data);
    printf("first three: %f, %f, %f\n", image, image[1], image[2]);
    allocator.unmap(buffer);

    allocator.destroy(buffer);
    allocator.deinit();
    context.deinit();
}
