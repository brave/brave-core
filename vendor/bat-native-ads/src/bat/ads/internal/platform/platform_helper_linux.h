/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_LINUX_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_LINUX_H_

#include <string>

#include "bat/ads/internal/platform/platform_helper.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ads {

class PlatformHelperLinux final : public PlatformHelper {
 public:
  PlatformHelperLinux(const PlatformHelperLinux&) = delete;
  PlatformHelperLinux& operator=(const PlatformHelperLinux&) = delete;

  static PlatformHelperLinux* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<PlatformHelperLinux>;

  PlatformHelperLinux();
  ~PlatformHelperLinux() override;

  // PlatformHelper impl
  bool IsMobile() const override;
  std::string GetPlatformName() const override;
  PlatformType GetPlatform() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_LINUX_H_
