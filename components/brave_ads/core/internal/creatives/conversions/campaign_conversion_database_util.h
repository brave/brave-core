/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_UTIL_H_

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"

namespace brave_ads::database {

void PurgeExpiredCampaignConversions();

void SaveCampaignConversions(
    const CampaignConversionList& campaign_conversions);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_UTIL_H_
