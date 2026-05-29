/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"

namespace blink {

class BraveScrubWebGpuAdapterInfo {
 public:
  BraveScrubWebGpuAdapterInfo(ExecutionContext* context,
                              String& vendor,
                              String& architecture,
                              String& device) {
    BraveFarblingLevel level = brave::GetBraveFarblingLevelFor(
        context, ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
        BraveFarblingLevel::OFF);
    auto farble = [&](const String& s) -> String {
      switch (level) {
        case BraveFarblingLevel::OFF:
          return s;
        case BraveFarblingLevel::BALANCED:
          return base::FeatureList::IsEnabled(
                     blink::features::kWebGLBalancedFingerprintingProtection)
                     ? String()
                     : s;
        case BraveFarblingLevel::MAXIMUM:
          return String();
      }
      return s;
    };
    vendor = farble(vendor);
    architecture = farble(architecture);
    device = farble(device);
  }

  ~BraveScrubWebGpuAdapterInfo() = default;
};

}  // namespace blink

#define BRAVE_SCRUB_WEBGPU_ADAPTER_INFO                             \
  BraveScrubWebGpuAdapterInfo(gpu_->GetExecutionContext(), vendor_, \
                              architecture_, device_);

#include <third_party/blink/renderer/modules/webgpu/gpu_adapter.cc>

#undef BRAVE_SCRUB_WEBGPU_ADAPTER_INFO
