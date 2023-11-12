// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <cassert>

#include <nvvk/context_vk.hpp>
#include <nvvk/structs_vk.hpp>  // vulkan使用sType成员变量来识别void*指向什么结构, 用nvvk::make简化这部分的初始化

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

    assert(acceleration_structure_feature.accelerationStructure == VK_TRUE && ray_query_feature.rayQuery == VK_TRUE);


    context.deinit();
}
