/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include <cstddef>
#include <optional>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_flag_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_flag_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"

namespace brave_ads {

namespace {

// Ensure the user has at least `kMinimumConfirmationTokenThreshold` tokens
// before an ad is served.
constexpr size_t kMinimumConfirmationTokenThreshold = 10;

bool IsAdTypeWithinRollingTimeConstraint(const AdType type,
                                         const base::TimeDelta time_constraint,
                                         const size_t cap) {
  if (cap == 0) {
    // If the cap is set to 0, then there is no time constraint.
    return true;
  }

  const std::vector<base::Time> history =
      GetCachedAdEvents(type, ConfirmationType::kServedImpression);

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint, cap);
}

}  // namespace

bool HasFullScreenModePermission() {
  if (!kShouldOnlyServeAdsInWindowedMode.Get()) {
    return true;
  }

  if (PlatformHelper::GetInstance().IsMobile()) {
    return true;
  }

  if (!IsBrowserInFullScreenMode()) {
    return true;
  }

  BLOG(2, "Full screen mode");
  return false;
}

bool HasIssuersPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (HasIssuers()) {
    return true;
  }

  BLOG(2, "Missing issuers");
  return false;
}

bool HasCommandLinePermission() {
  if (!IsProductionEnvironment()) {
    // Always respect cap for staging environment.
    return true;
  }

  if (!DidOverrideCommandLine()) {
    return true;
  }

  BLOG(2, "Command-line arg is not supported");
  return false;
}

bool HasConfirmationTokensPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (ConfirmationTokenCount() >= kMinimumConfirmationTokenThreshold) {
    return true;
  }

  BLOG(2, "You do not have enough confirmation tokens");
  return false;
}

bool HasUserActivityPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (PlatformHelper::GetInstance().GetType() == PlatformType::kIOS) {
    return true;
  }

  if (WasUserActive()) {
    return true;
  }

  BLOG(2, "User was inactive");
  return false;
}

bool HasSearchResultAdsPerHourPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kSearchResultAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/kMaximumSearchResultAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed search result ads per hour");
    return false;
  }

  return true;
}

bool HasSearchResultAdsPerDayPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kSearchResultAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumSearchResultAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed search result ads per day");
    return false;
  }

  return true;
}

bool HasNewTabPageAdsPerHourPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNewTabPageAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/kMaximumNewTabPageAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed new tab page ads per hour");
    return false;
  }

  return true;
}

bool HasNewTabPageAdsPerDayPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNewTabPageAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumNewTabPageAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed new tab page ads per day");
    return false;
  }

  return true;
}

bool HasNewTabPageAdMinimumWaitTimePermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNewTabPageAd,
          /*time_constraint=*/kNewTabPageAdMinimumWaitTime.Get(),
          /*cap=*/1)) {
    BLOG(2,
         "New tab page ad cannot be shown as minimum wait time has not passed");
    return false;
  }

  return true;
}

bool HasNotificationAdsPerHourPermission() {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never exceed the
    // maximum ads per hour.
    return true;
  }

  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNotificationAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/GetMaximumNotificationAdsPerHour())) {
    BLOG(2, "You have exceeded the allowed notification ads per hour");
    return false;
  }

  return true;
}

bool HasNotificationAdsPerDayPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNotificationAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumNotificationAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed notification ads per day");
    return false;
  }

  return true;
}

bool HasNotificationAdMinimumWaitTimePermission() {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never be served before
    // the minimum wait time has passed.
    return true;
  }

  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kNotificationAd,
          /*time_constraint=*/base::Hours(1) /
              GetMaximumNotificationAdsPerHour(),
          /*cap=*/1)) {
    BLOG(2,
         "Notification ad cannot be shown as minimum wait time has not passed");
    return false;
  }

  return true;
}

bool HasNetworkConnectionPermission() {
  if (!kShouldOnlyServeAdsWithValidInternetConnection.Get()) {
    return true;
  }

  if (IsNetworkConnectionAvailable()) {
    return true;
  }

  BLOG(2, "Network connection is unavailable");
  return false;
}

bool HasMediaPermission() {
  if (!kShouldOnlyServeAdsIfMediaIsNotPlaying.Get()) {
    return true;
  }

  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetVisible();
  if (!tab) {
    return true;
  }

  if (!TabManager::GetInstance().IsPlayingMedia(tab->id)) {
    return true;
  }

  BLOG(2, "Media is playing");
  return false;
}

bool HasDoNotDisturbPermission() {
  if (PlatformHelper::GetInstance().GetType() != PlatformType::kAndroid) {
    return true;
  }

  if (BrowserManager::GetInstance().IsActive() &&
      BrowserManager::GetInstance().IsInForeground()) {
    return true;
  }

  base::Time::Exploded exploded;
  base::Time::Now().LocalExplode(&exploded);

  if (exploded.hour >= kDoNotDisturbToHour.Get() &&
      exploded.hour < kDoNotDisturbFromHour.Get()) {
    return true;
  }

  BLOG(2, "Should not disturb");
  return false;
}

bool HasAllowNotificationsPermission() {
  if (!CanShowNotificationAds()) {
    BLOG(2, "System notifications not allowed");
    return false;
  }

  return true;
}

bool HasCatalogPermission() {
  if (!DoesCatalogExist()) {
    BLOG(2, "Catalog does not exist");
    return false;
  }

  if (HasCatalogExpired()) {
    BLOG(2, "Catalog has expired");
    return false;
  }

  return true;
}

bool HasInlineContentAdsPerHourPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kInlineContentAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/kMaximumInlineContentAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed inline content ads per hour");
    return false;
  }

  return true;
}

bool HasInlineContentAdsPerDayPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kInlineContentAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumInlineContentAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed inline content ads per day");
    return false;
  }

  return true;
}

bool HasPromotedContentAdsPerHourPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kPromotedContentAd, /*time_constraint=*/base::Hours(1),
          /*cap=*/kMaximumPromotedContentAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed promoted content ads per hour");
    return false;
  }

  return true;
}

bool HasPromotedContentAdsPerDayPermission() {
  if (!IsAdTypeWithinRollingTimeConstraint(
          AdType::kPromotedContentAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumPromotedContentAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed promoted content ads per day");
    return false;
  }

  return true;
}

bool HasBrowserIsActivePermission() {
  if (!kShouldOnlyServeAdsIfBrowserIsActive.Get()) {
    return true;
  }

  if (BrowserManager::GetInstance().IsActive() &&
      BrowserManager::GetInstance().IsInForeground()) {
    return true;
  }

  BLOG(2, "Browser window is not active");
  return false;
}

}  // namespace brave_ads
