/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

// This class allows to scrub the various device identifiers in the
// GPUAdapterInfo depending on the farbling level.
class BraveScrubWebGpuAdapterInfo {
 public:
  BraveScrubWebGpuAdapterInfo(ExecutionContext* context,
                              String& vendor,
                              String& architecture,
                              String& device)
      : farbling_level_(brave::GetBraveFarblingLevelFor(
            context,
            ContentSettingsType::BRAVE_WEBCOMPAT_WEBGPU,
            BraveFarblingLevel::OFF)),
        reset_vendor_(&vendor, ApplyFarbling(vendor)),
        reset_architecture_(&architecture, ApplyFarbling(architecture)),
        reset_device_(&device, ApplyFarbling(device)) {}

  ~BraveScrubWebGpuAdapterInfo() = default;

 private:
  String ApplyFarbling(const String& s) {
    switch (farbling_level_) {
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
  }

  const BraveFarblingLevel farbling_level_;
  const base::AutoReset<String> reset_vendor_;
  const base::AutoReset<String> reset_architecture_;
  const base::AutoReset<String> reset_device_;
};
}  // namespace

}  // namespace blink

#define BRAVE_SCRUB_WEBGPU_ADAPTER_INFO                             \
  BraveScrubWebGpuAdapterInfo(gpu_->GetExecutionContext(), vendor_, \
                              architecture_, device_);

#include <third_party/blink/renderer/modules/webgpu/gpu_adapter.cc>

#undef BRAVE_SCRUB_WEBGPU_ADAPTER_INFO
