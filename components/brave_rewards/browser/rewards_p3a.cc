/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {
namespace p3a {

#if !BUILDFLAG(IS_ANDROID)
namespace {

// The maximum time difference allowed between the rewards panel opening action
// (which may have enabled rewards) and the actual enabling of rewards. This is
// to ensure that there is a clear connection between the action and the reward
// enabling.
constexpr base::TimeDelta kMaxEnabledCauseTriggerTime = base::Minutes(1);

}  // namespace
#endif  // !BUILDFLAG(IS_ANDROID)

const char kEnabledSourceHistogramName[] = "Brave.Rewards.EnabledSource";
const char kInlineTipTriggerHistogramName[] = "Brave.Rewards.InlineTipTrigger";
const char kToolbarButtonTriggerHistogramName[] =
    "Brave.Rewards.ToolbarButtonTrigger";
const char kTipsSentHistogramName[] = "Brave.Rewards.TipsSent";
const char kAutoContributionsStateHistogramName[] =
    "Brave.Rewards.AutoContributionsState.3";
const char kAdsEnabledDurationHistogramName[] =
    "Brave.Rewards.AdsEnabledDuration";
const int kTipsSentBuckets[] = {1, 3};

void RecordAutoContributionsState(bool ac_enabled) {
  UMA_HISTOGRAM_EXACT_LINEAR(kAutoContributionsStateHistogramName, ac_enabled,
                             2);
}

void RecordTipsSent(size_t tip_count) {
  DCHECK_GE(tip_count, 0u);

  if (tip_count == 0) {
    return;
  }

  p3a_utils::RecordToHistogramBucket(kTipsSentHistogramName, kTipsSentBuckets,
                                     tip_count);
}

void RecordNoWalletCreatedForAllMetrics() {
  UMA_HISTOGRAM_EXACT_LINEAR(kTipsSentHistogramName, INT_MAX - 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kAutoContributionsStateHistogramName, INT_MAX - 1,
                             2);
}

void RecordAdsEnabledDuration(PrefService* prefs, bool ads_enabled) {
  base::Time enabled_timestamp = prefs->GetTime(prefs::kAdsEnabledTimestamp);
  base::TimeDelta enabled_time_delta =
      prefs->GetTimeDelta(prefs::kAdsEnabledTimeDelta);
  AdsEnabledDuration enabled_duration;

  if (enabled_timestamp.is_null()) {
    // No previous timestamp, so record one of the non-duration states.
    if (ads_enabled) {
      // Ads have been enabled.
      // Remember when so we can measure the duration on later changes.
      prefs->SetTime(prefs::kAdsEnabledTimestamp, base::Time::Now());
    }
  } else {
    // Previous timestamp available.
    if (!ads_enabled) {
      // Ads have been disabled. Record the duration they were on.
      enabled_time_delta = base::Time::Now() - enabled_timestamp;
      VLOG(1) << "Rewards disabled after " << enabled_time_delta;
      // Null the timestamp so we're ready for a fresh measurement.
      // Store the enabled time delta so we can keep reporting the duration.
      prefs->SetTime(prefs::kAdsEnabledTimestamp, base::Time());
      prefs->SetTimeDelta(prefs::kAdsEnabledTimeDelta, enabled_time_delta);
    }
  }
  // Set the threshold at three units so each bin represents the
  // nominal value as an order-of-magnitude: more than three days
  // is a week, more than three weeks is a month, and so on.
  constexpr int threshold = 3;
  constexpr int days_per_week = 7;
  constexpr double days_per_month = 30.44;  // average length
  if (ads_enabled) {
    enabled_duration = AdsEnabledDuration::kStillEnabled;
  } else if (enabled_time_delta.is_zero()) {
    enabled_duration = AdsEnabledDuration::kNever;
  } else if (enabled_time_delta < base::Hours(threshold)) {
    enabled_duration = AdsEnabledDuration::kHours;
  } else if (enabled_time_delta < base::Days(threshold)) {
    enabled_duration = AdsEnabledDuration::kDays;
  } else if (enabled_time_delta < base::Days(threshold * days_per_week)) {
    enabled_duration = AdsEnabledDuration::kWeeks;
  } else if (enabled_time_delta < base::Days(threshold * days_per_month)) {
    enabled_duration = AdsEnabledDuration::kMonths;
  } else {
    enabled_duration = AdsEnabledDuration::kQuarters;
  }

  UMA_HISTOGRAM_ENUMERATION(kAdsEnabledDurationHistogramName, enabled_duration);
}

ConversionMonitor::ConversionMonitor() = default;
ConversionMonitor::~ConversionMonitor() = default;

void ConversionMonitor::RecordPanelTrigger(PanelTrigger trigger) {
#if !BUILDFLAG(IS_ANDROID)
  if (trigger == PanelTrigger::kInlineTip) {
    UMA_HISTOGRAM_EXACT_LINEAR(kInlineTipTriggerHistogramName, 1, 2);
  } else if (trigger == PanelTrigger::kToolbarButton) {
    UMA_HISTOGRAM_EXACT_LINEAR(kToolbarButtonTriggerHistogramName, 1, 2);
  }
  last_trigger_ = trigger;
  last_trigger_time_ = base::Time::Now();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void ConversionMonitor::RecordRewardsEnable() {
#if !BUILDFLAG(IS_ANDROID)
  // Suspend the other two metrics to prevent overlapping
  // data from being sent once the "rewards enabled source" metric is recorded.
  UMA_HISTOGRAM_EXACT_LINEAR(kToolbarButtonTriggerHistogramName, INT_MAX - 1,
                             2);
  UMA_HISTOGRAM_EXACT_LINEAR(kInlineTipTriggerHistogramName, INT_MAX - 1, 2);

  if (!last_trigger_.has_value() ||
      base::Time::Now() - last_trigger_time_ > kMaxEnabledCauseTriggerTime) {
    return;
  }

  UMA_HISTOGRAM_ENUMERATION(kEnabledSourceHistogramName, *last_trigger_);

  last_trigger_.reset();
  last_trigger_time_ = base::Time();
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace p3a
}  // namespace brave_rewards
