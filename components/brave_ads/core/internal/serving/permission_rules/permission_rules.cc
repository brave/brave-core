/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
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
#include "brave/components/brave_ads/core/internal/units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/user/user_attention/user_activity/user_activity_scoring_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/units/search_result_ad/search_result_ad_feature.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

// A one hour constraint for a given permission rule
constexpr base::TimeDelta kOneHour = base::Hours(1);

// A one day constraint for a given permission rule
constexpr base::TimeDelta kOneDay = base::Days(1);

// The minimum wait time cap for minimum wait permission rules
constexpr int kMinimumWaitTimeCap = 1;

bool IsAdTypeInTimeRangeAllowed(const AdType& type,
                                base::TimeDelta constraint,
                                size_t cap) {
  const std::vector<base::Time> history =
      GetCachedAdEvents(type, ConfirmationType::kServed);

  return DoesHistoryRespectRollingTimeConstraint(history, constraint,
                                                 /*cap=*/cap);
}

}  // namespace

bool ShouldAllowFullScreenMode() {
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

bool ShouldAllowIssuers() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }
  if (HasIssuers()) {
    return true;
  }

  BLOG(2, "Missing issuers");
  return false;
}

bool ShouldAllowCommandLine() {
  if (!IsProductionEnvironment()) {
    // Always respect cap for staging environment
    return true;
  }
  if (!DidOverrideCommandLine()) {
    return true;
  }

  BLOG(2, "Command-line arg is not supported");
  return false;
}

bool ShouldAllowConfirmationTokens() {
  constexpr int kMinimumConfirmationTokenThreshold = 10;
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }
  if (ConfirmationTokenCount() >= kMinimumConfirmationTokenThreshold) {
    return true;
  }

  BLOG(2, "You do not have enough confirmation tokens");
  return false;
}

bool ShouldAllowUserActivity() {
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

bool ShouldAllowSearchResultAdsPerDay() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kSearchResultAd, kOneDay,
          /*cap=*/kMaximumSearchResultAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed search result ads per day");
    return false;
  }
  return true;
}

bool ShouldAllowSearchResultAdsPerHour() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kSearchResultAd, kOneHour,
          /*cap=*/kMaximumSearchResultAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed search result ads per hour");
    return false;
  }
  return true;
}

bool ShouldAllowNewTabPageAdsPerDay() {
  if (!IsAdTypeInTimeRangeAllowed(AdType::kNewTabPageAd, kOneDay,
                                  /*cap=*/kMaximumNewTabPageAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed new tab page ads per day");
    return false;
  }
  return true;
}

bool ShouldAllowNewTabPageAdMinimumWaitTime() {
  if (!IsAdTypeInTimeRangeAllowed(AdType::kNewTabPageAd,
                                  kNewTabPageAdMinimumWaitTime.Get(),
                                  /*cap=*/kMinimumWaitTimeCap)) {
    BLOG(2,
         "New tab page ad cannot be shown as minimum wait time has not passed");
    return false;
  }
  return true;
}

bool ShouldAllowNewTabPageAdsPerHour() {
  if (!IsAdTypeInTimeRangeAllowed(AdType::kNewTabPageAd, kOneHour,
                                  /*cap=*/kMaximumNewTabPageAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed new tab page ads per hour");
    return false;
  }
  return true;
}

bool ShouldAllowNotificationAdsPerHour() {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never exceed the
    // maximum ads per hour
    return true;
  }

  if (!IsAdTypeInTimeRangeAllowed(AdType::kNotificationAd, kOneHour,
                                  /*cap=*/GetMaximumNotificationAdsPerHour())) {
    BLOG(2, "You have exceeded the allowed notification ads per hour");
    return false;
  }
  return true;
}

bool ShouldAllowNotificationAdsPerDay() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kNotificationAd, kOneDay,
          /*cap=*/kMaximumNotificationAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed notification ads per day");
    return false;
  }
  return true;
}

bool ShouldAllowNotificationAdMinimumWaitTime() {
  if (PlatformHelper::GetInstance().IsMobile()) {
    // Ads are periodically served on mobile so they will never be served before
    // the minimum wait time has passed
    return true;
  }

  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kNotificationAd,
          /*time_constraint=*/kOneHour / GetMaximumNotificationAdsPerHour(),
          /*cap=*/kMinimumWaitTimeCap)) {
    BLOG(2,
         "Notification ad cannot be shown as minimum wait time has not passed");
    return false;
  }

  return true;
}

bool ShouldAllowNetworkConnection() {
  if (!kShouldOnlyServeAdsWithValidInternetConnection.Get()) {
    return true;
  }
  if (IsNetworkConnectionAvailable()) {
    return true;
  }

  BLOG(2, "Network connection is unavailable");
  return false;
}

bool ShouldAllowMedia() {
  if (!kShouldOnlyServeAdsIfMediaIsNotPlaying.Get()) {
    return true;
  }
  const absl::optional<TabInfo> tab = TabManager::GetInstance().GetVisible();
  if (!tab) {
    return true;
  }
  if (!TabManager::GetInstance().IsPlayingMedia(tab->id)) {
    return true;
  }

  BLOG(2, "Media is playing");
  return false;
}

bool ShouldAllowDoNotDisturb() {
  if (PlatformHelper::GetInstance().GetType() != PlatformType::kAndroid) {
    return true;
  }
  if (BrowserManager::GetInstance().IsActive() &&
      BrowserManager::GetInstance().IsInForeground()) {
    return true;
  }

  const base::Time time = base::Time::Now();
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  if (exploded.hour >= kDoNotDisturbToHour.Get() &&
      exploded.hour < kDoNotDisturbFromHour.Get()) {
    return true;
  }

  BLOG(2, "Should not disturb");
  return false;
}

bool ShouldAllowAllowNotifications() {
  if (!CanShowNotificationAds()) {
    BLOG(2, "System notifications not allowed");
    return false;
  }

  return true;
}

bool ShouldAllowCatalog() {
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

bool ShouldAllowInlineContentAdsPerDay() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kInlineContentAd, kOneDay,
          /*cap=*/kMaximumInlineContentAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed inline content ads per day");
    return false;
  }

  return true;
}

bool ShouldAllowInlineContentAdsPerHour() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kInlineContentAd, kOneHour,
          /*cap=*/kMaximumInlineContentAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed inline content ads per hour");
    return false;
  }

  return true;
}

bool ShouldAllowPromotedContentAdsPerDay() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kPromotedContentAd, kOneDay,
          /*cap=*/kMaximumPromotedContentAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed promoted content ads per day");
    return false;
  }

  return true;
}

bool ShouldAllowPromotedContentAdsPerHour() {
  if (!IsAdTypeInTimeRangeAllowed(
          AdType::kPromotedContentAd, kOneHour,
          /*cap=*/kMaximumPromotedContentAdsPerHour.Get())) {
    BLOG(2, "You have exceeded the allowed promoted content ads per hour");
    return false;
  }

  return true;
}

bool ShouldAllowBrowserIsActive() {
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
