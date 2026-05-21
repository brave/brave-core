/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_util.h"

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"

namespace brave_ads {

bool IsMobile() {
  const OperatingSystemType type = OperatingSystem::GetInstance().GetType();
  return type == OperatingSystemType::kAndroid ||
         type == OperatingSystemType::kIOS;
}

}  // namespace brave_ads
