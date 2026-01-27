/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_CONSTANTS_H_

#include "brave/components/brave_ads/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

inline constexpr char kNewTabPageAdTypeKey[] = "type";
inline constexpr char kNewTabPageAdPlacementIdKey[] = "placement_id";
inline constexpr char kNewTabPageAdCreativeInstanceIdKey[] =
    "creative_instance_id";
inline constexpr char kNewTabPageAdCreativeSetIdKey[] = "creative_set_id";
inline constexpr char kNewTabPageAdCampaignIdKey[] = "campaign_id";
inline constexpr char kNewTabPageAdAdvertiserIdKey[] = "advertiser_id";
inline constexpr char kNewTabPageAdSegmentKey[] = "segment";
inline constexpr char kNewTabPageAdCompanyNameKey[] = "company_name";
inline constexpr char kNewTabPageAdAltKey[] = "alt";
inline constexpr char kNewTabPageAdTargetUrlKey[] = "target_url";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_CONSTANTS_H_
