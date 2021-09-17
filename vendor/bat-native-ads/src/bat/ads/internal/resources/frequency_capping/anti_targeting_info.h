/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_INFO_H_

#include <cstdint>

#include "bat/ads/internal/resources/frequency_capping/anti_targeting_info_aliases.h"

namespace ads {
namespace resource {

struct AntiTargetingInfo final {
 public:
  AntiTargetingInfo();
  AntiTargetingInfo(const AntiTargetingInfo& info);
  ~AntiTargetingInfo();

  uint16_t version = 0;
  AntiTargetingMap sites;
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_INFO_H_
