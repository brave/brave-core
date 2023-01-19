/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>

class GURL;

namespace base {
class Value;
}  // namespace base

namespace ads::resource {

using AntiTargetingSiteList = std::set<GURL>;
using AntiTargetingMap = std::map<std::string, AntiTargetingSiteList>;

struct AntiTargetingInfo final {
  AntiTargetingInfo();

  AntiTargetingInfo(const AntiTargetingInfo& other);
  AntiTargetingInfo& operator=(const AntiTargetingInfo& other);

  AntiTargetingInfo(AntiTargetingInfo&& other) noexcept;
  AntiTargetingInfo& operator=(AntiTargetingInfo&& other) noexcept;

  ~AntiTargetingInfo();

  static std::unique_ptr<AntiTargetingInfo> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  uint16_t version = 0;
  AntiTargetingMap sites;
};

}  // namespace ads::resource

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
