/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"

// clang-format off
#define BRAVE_CREATE_WEBGPU_ADAPTER_IMPL                                       \
  ExecutionContext* context = gpu_->GetExecutionContext();                     \
  BraveFarblingLevel level = brave::GetBraveFarblingLevelFor(                  \
      context, ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,                     \
      BraveFarblingLevel::OFF);                                                \
  auto farble = [&](const String& s) -> String {                               \
    switch (level) {                                                           \
      case BraveFarblingLevel::OFF:                                            \
        return s;                                                              \
      case BraveFarblingLevel::BALANCED:                                       \
        return base::FeatureList::IsEnabled(                                   \
                   blink::features::kWebGLBalancedFingerprintingProtection)    \
                   ? String()                                                  \
                   : s;                                                        \
      case BraveFarblingLevel::MAXIMUM:                                        \
        return String();                                                       \
    }                                                                          \
    return s;                                                                  \
  };                                                                           \
  base::AutoReset<String> reset_vendor(&vendor_, farble(vendor_));             \
  base::AutoReset<String> reset_arch(&architecture_, farble(architecture_));   \
  base::AutoReset<String> reset_device(&device_, farble(device_));
// clang-format on

#include <third_party/blink/renderer/modules/webgpu/gpu_adapter.cc>

#undef BRAVE_CREATE_WEBGPU_ADAPTER_IMPL
