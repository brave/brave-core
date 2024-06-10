/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_INFO_H_

#include <optional>
#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_info.h"

namespace brave_ads {

struct ConversionResourceInfo final {
  ConversionResourceInfo();

  ConversionResourceInfo(const ConversionResourceInfo&) = delete;
  ConversionResourceInfo& operator=(const ConversionResourceInfo&) = delete;

  ConversionResourceInfo(ConversionResourceInfo&& other) noexcept;
  ConversionResourceInfo& operator=(ConversionResourceInfo&& other) noexcept;

  ~ConversionResourceInfo();

  static base::expected<ConversionResourceInfo, std::string> CreateFromValue(
      base::Value::Dict dict);

  std::optional<int> version;
  ConversionResourceIdPatternMap id_patterns;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_RESOURCE_CONVERSION_RESOURCE_INFO_H_
