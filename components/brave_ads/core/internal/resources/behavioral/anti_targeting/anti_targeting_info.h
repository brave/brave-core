/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>

#include "base/types/expected.h"
#include "base/values.h"

class GURL;

namespace brave_ads::resource {

using AntiTargetingSiteList = std::set<GURL>;
using AntiTargetingMap = std::map<std::string, AntiTargetingSiteList>;

struct AntiTargetingInfo final {
  AntiTargetingInfo();

  AntiTargetingInfo(const AntiTargetingInfo&) = delete;
  AntiTargetingInfo& operator=(const AntiTargetingInfo&) = delete;

  AntiTargetingInfo(AntiTargetingInfo&&) noexcept;
  AntiTargetingInfo& operator=(AntiTargetingInfo&&) noexcept;

  ~AntiTargetingInfo();

  static base::expected<AntiTargetingInfo, std::string> CreateFromValue(
      base::Value::Dict dict);

  uint16_t version = 0;
  AntiTargetingMap sites;
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_INFO_H_
