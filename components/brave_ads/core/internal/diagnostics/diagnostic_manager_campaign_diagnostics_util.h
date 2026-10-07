/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CAMPAIGN_DIAGNOSTICS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CAMPAIGN_DIAGNOSTICS_UTIL_H_

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/segments/segment_types.h"

namespace brave_ads {

void GetNotificationAdCampaignsCallback(
    GetCampaignsDiagnosticsCallback callback,
    bool success,
    const SegmentList& segments,
    CreativeNotificationAdList creative_ads);

void GetNewTabPageAdCampaignsCallback(GetCampaignsDiagnosticsCallback callback,
                                      bool success,
                                      const SegmentList& segments,
                                      CreativeNewTabPageAdList creative_ads);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CAMPAIGN_DIAGNOSTICS_UTIL_H_
