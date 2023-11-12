// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
#include <iostream>
#include <nvvk/context_vk.hpp>

int main(int argc, const char** argv) {
    nvvk::ContextCreateInfo deviceInfo; // Context初始化的hints
    nvvk::Context context; // instance, device, physical device, and queues
    context.init(deviceInfo);

    vkAllocateCommandBuffers(context, nullptr, nullptr); // 测试VK_LAYER_KHRONOS_validation

    context.deinit();
}
