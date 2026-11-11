/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"

namespace brave_ads {

CampaignConversionInfo::CampaignConversionInfo() = default;

CampaignConversionInfo::CampaignConversionInfo(
    const CampaignConversionInfo& other) = default;

CampaignConversionInfo& CampaignConversionInfo::operator=(
    const CampaignConversionInfo& other) = default;

CampaignConversionInfo::CampaignConversionInfo(
    CampaignConversionInfo&& other) noexcept = default;

CampaignConversionInfo& CampaignConversionInfo::operator=(
    CampaignConversionInfo&& other) noexcept = default;

CampaignConversionInfo::~CampaignConversionInfo() = default;

bool CampaignConversionInfo::IsValid() const {
  return !id.empty() && !url_pattern.empty() &&
         !observation_window.is_negative() && expire_at;
}

}  // namespace brave_ads
