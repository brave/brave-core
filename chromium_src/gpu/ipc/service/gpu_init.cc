/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_VULKAN_GL_RENDERER_AND_DEVICE_NAME_CHECK                \
  const base::FeatureParam<std::string> disable_patterns_brave(       \
      &features::kVulkan, "disable_by_gl_renderer",                   \
      "*Mali-G72*|*Mali-G?? M*");                                     \
  if (MatchGLRenderer(gpu_info_, disable_patterns_brave.Get()))       \
    return false;                                                     \
  const base::FeatureParam<std::string> enable_by_device_name_brave(  \
      &features::kVulkan, "enable_by_device_name", "Adreno*630");     \
  if (!use_swiftshader && !forced_native &&                           \
      !CheckVulkanCompabilities(                                      \
          vulkan_implementation_->GetVulkanInstance()->vulkan_info(), \
          gpu_info_, enable_by_device_name_brave.Get())) {            \
    vulkan_implementation_.reset();                                   \
    return false;                                                     \
  }

#include "src/gpu/ipc/service/gpu_init.cc"

#undef BRAVE_VULKAN_GL_RENDERER_AND_DEVICE_NAME_CHECK
