/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_

namespace brave_ads {

bool ShouldAllowFullScreenMode();
bool ShouldAllowIssuers();
bool ShouldAllowCommandLine();
bool ShouldAllowConfirmationTokens();
bool ShouldAllowUserActivity();
bool ShouldAllowSearchResultAdsPerDay();
bool ShouldAllowSearchResultAdsPerHour();
bool ShouldAllowNewTabPageAdsPerDay();
bool ShouldAllowNewTabPageAdMinimumWaitTime();
bool ShouldAllowNewTabPageAdsPerHour();
bool ShouldAllowNotificationAdsPerHour();
bool ShouldAllowNotificationAdsPerDay();
bool ShouldAllowNotificationAdMinimumWaitTime();
bool ShouldAllowNetworkConnection();
bool ShouldAllowMedia();
bool ShouldAllowDoNotDisturb();
bool ShouldAllowAllowNotifications();
bool ShouldAllowCatalog();
bool ShouldAllowInlineContentAdsPerDay();
bool ShouldAllowInlineContentAdsPerHour();
bool ShouldAllowPromotedContentAdsPerDay();
bool ShouldAllowPromotedContentAdsPerHour();
bool ShouldAllowBrowserIsActive();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_
