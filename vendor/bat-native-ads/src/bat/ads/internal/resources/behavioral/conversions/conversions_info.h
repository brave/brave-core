/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_

#include <memory>
#include <string>

#include "bat/ads/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"

namespace base {
class Value;
}  // namespace base

namespace ads::resource {

struct ConversionsInfo final {
  ConversionsInfo();

  ConversionsInfo(const ConversionsInfo& other);
  ConversionsInfo& operator=(const ConversionsInfo& other);

  ConversionsInfo(ConversionsInfo&& other) noexcept;
  ConversionsInfo& operator=(ConversionsInfo&& other) noexcept;

  ~ConversionsInfo();

  static std::unique_ptr<ConversionsInfo> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  int version = 0;
  ConversionIdPatternMap id_patterns;
};

}  // namespace ads::resource

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_INFO_H_
