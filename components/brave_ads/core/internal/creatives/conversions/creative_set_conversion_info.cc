/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"

namespace brave_ads {

CreativeSetConversionInfo::CreativeSetConversionInfo() = default;

CreativeSetConversionInfo::CreativeSetConversionInfo(
    const CreativeSetConversionInfo& other) = default;

CreativeSetConversionInfo& CreativeSetConversionInfo::operator=(
    const CreativeSetConversionInfo& other) = default;

CreativeSetConversionInfo::CreativeSetConversionInfo(
    CreativeSetConversionInfo&& other) noexcept = default;

CreativeSetConversionInfo& CreativeSetConversionInfo::operator=(
    CreativeSetConversionInfo&& other) noexcept = default;

CreativeSetConversionInfo::~CreativeSetConversionInfo() = default;

bool CreativeSetConversionInfo::IsValid() const {
  return !id.empty() && !url_pattern.empty() &&
         !observation_window.is_negative() && expire_at;
}

}  // namespace brave_ads
