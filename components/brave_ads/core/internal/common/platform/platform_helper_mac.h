/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MAC_H_

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

#include <string>

#include "base/no_destructor.h"

namespace brave_ads {

class PlatformHelperMac final : public PlatformHelper {
 public:
  PlatformHelperMac(const PlatformHelperMac&) = delete;
  PlatformHelperMac& operator=(const PlatformHelperMac&) = delete;

 protected:
  friend class base::NoDestructor<PlatformHelperMac>;

  PlatformHelperMac();

 private:
  // PlatformHelper:
  bool IsMobile() const override;
  std::string GetName() const override;
  PlatformType GetType() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_MAC_H_
