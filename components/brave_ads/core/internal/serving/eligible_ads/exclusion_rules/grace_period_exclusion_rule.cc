/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/grace_period_exclusion_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-data-view.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

GracePeriodExclusionRule::GracePeriodExclusionRule() = default;

GracePeriodExclusionRule::~GracePeriodExclusionRule() = default;

std::string GracePeriodExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool GracePeriodExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (ShouldDebug()) {
    // Ignore the grace period when debug mode is enabled to allow testing.
    return true;
  }

  if (creative_ad.metric_type == mojom::NewTabPageAdMetricType::kDisabled) {
    // Ignore the grace period when metrics are disabled.
    return true;
  }

  const base::Time first_run_at = GetLocalStateTimePref(prefs::kFirstRunAt);
  const base::Time grace_period_end_at =
      first_run_at + GetProfileTimeDeltaPref(prefs::kGracePeriod);

  if (base::Time::Now() < grace_period_end_at) {
    BLOG(1, "campaignId " << creative_ad.campaign_id
                          << " is within the grace period which ends "
                          << FriendlyDateAndTime(grace_period_end_at));
    return false;
  }

  return true;
}

}  // namespace brave_ads
