/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/metrics/field_trial_params.h"
#include "gpu/config/gpu_finch_features.h"

#include <string>

namespace features {

const base::FeatureParam<std::string> kVulkanBraveBlockListByBoard{
    &kVulkan, "BraveBlockListByBoard",
    "RM67*|RM68*|k68*|mt67*|oppo67*|oppo68*|QM215|rk30sdk|secret|maltose|"
    "rosemary|HLTE322E*|lisbon|lancelot|Infinix-X695|"
    "g2062upt_v1_gd_sh2_gq_eea_r|cannong|TECNO-CG8"};

}  // namespace features

#endif

#define BRAVE_VULKAN_BLOCK_LIST_DEVICE_CHECK               \
  if (IsDeviceBlocked(build_info->board(),                 \
                      kVulkanBraveBlockListByBoard.Get())) \
    return false;

#include "src/gpu/config/gpu_finch_features.cc"

#undef BRAVE_VULKAN_BLOCK_LIST_DEVICE_CHECK
