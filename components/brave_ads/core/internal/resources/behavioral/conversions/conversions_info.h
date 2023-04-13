/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"

namespace base {
class Value;
}  // namespace base

namespace brave_ads::resource {

struct ConversionsInfo final {
  ConversionsInfo();

  ConversionsInfo(const ConversionsInfo&);
  ConversionsInfo& operator=(const ConversionsInfo&);

  ConversionsInfo(ConversionsInfo&&) noexcept;
  ConversionsInfo& operator=(ConversionsInfo&&) noexcept;

  ~ConversionsInfo();

  static std::unique_ptr<ConversionsInfo> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  int version = 0;
  ConversionIdPatternMap id_patterns;
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
