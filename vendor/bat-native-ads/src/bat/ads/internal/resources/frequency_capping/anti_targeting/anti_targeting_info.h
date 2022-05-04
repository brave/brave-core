/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_INFO_H_

#include <cstdint>
#include <memory>
#include <string>

#include "bat/ads/internal/resources/frequency_capping/anti_targeting/anti_targeting_info_aliases.h"

namespace base {
class Value;
}  // namespace base

namespace ads {
namespace resource {

struct AntiTargetingInfo final {
 public:
  AntiTargetingInfo();
  AntiTargetingInfo(const AntiTargetingInfo& info);
  ~AntiTargetingInfo();

  static std::unique_ptr<AntiTargetingInfo> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  uint16_t version = 0;
  AntiTargetingMap sites;
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
