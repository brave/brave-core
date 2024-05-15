/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_

namespace brave_ads {

bool HasFullScreenModePermission();

bool HasIssuersPermission();

bool HasCommandLinePermission();

bool HasConfirmationTokensPermission();

bool HasUserActivityPermission();

bool HasSearchResultAdsPerHourPermission();
bool HasSearchResultAdsPerDayPermission();

bool HasNewTabPageAdsPerHourPermission();
bool HasNewTabPageAdsPerDayPermission();
bool HasNewTabPageAdMinimumWaitTimePermission();

bool HasNotificationAdsPerHourPermission();
bool HasNotificationAdsPerDayPermission();
bool HasNotificationAdMinimumWaitTimePermission();

bool HasNetworkConnectionPermission();

bool HasMediaPermission();

bool HasDoNotDisturbPermission();

bool HasAllowNotificationsPermission();

bool HasCatalogPermission();

bool HasInlineContentAdsPerDayPermission();
bool HasInlineContentAdsPerHourPermission();

bool HasPromotedContentAdsPerHourPermission();
bool HasPromotedContentAdsPerDayPermission();

bool HasBrowserIsActivePermission();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_PERMISSION_RULES_H_
