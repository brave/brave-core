/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_

#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"

namespace brave_ads {

struct ConversionsInfo final {
  ConversionsInfo();

  ConversionsInfo(const ConversionsInfo&) = delete;
  ConversionsInfo& operator=(const ConversionsInfo&) = delete;

  ConversionsInfo(ConversionsInfo&& other) noexcept;
  ConversionsInfo& operator=(ConversionsInfo&& other) noexcept;

  ~ConversionsInfo();

  static base::expected<ConversionsInfo, std::string> CreateFromValue(
      base::Value::Dict dict);

  int version = 0;
  ConversionIdPatternMap id_patterns;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
