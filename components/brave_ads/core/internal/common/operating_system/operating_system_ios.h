/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_IOS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_IOS_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"

namespace brave_ads {

class OperatingSystemIos final : public OperatingSystem {
 public:
  OperatingSystemIos();

 private:
  // OperatingSystem:
  std::string GetName() const override;
  OperatingSystemType GetType() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_IOS_H_
