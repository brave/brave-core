/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_p3a.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards::p3a {

namespace {

// The maximum time difference allowed between the rewards panel opening action
// (which may have enabled rewards) and the actual enabling of rewards. This is
// to ensure that there is a clear connection between the action and the reward
// enabling.
constexpr base::TimeDelta kMaxEnabledCauseTriggerTime = base::Minutes(1);

constexpr base::TimeDelta kReportInterval = base::Days(1);

constexpr int kTipsSentBuckets[] = {0, 1, 3};

constexpr int kPanelCountBuckets[] = {5, 10, 50};

constexpr int kRewardsPageViewCountBuckets[] = {2, 5, 10, 50};

}  // namespace

void RecordAutoContributionsState(bool ac_enabled) {
  UMA_HISTOGRAM_EXACT_LINEAR(kAutoContributionsStateHistogramName, ac_enabled,
                             2);
}

void RecordTipsSent(size_t tip_count) {
  DCHECK_GE(tip_count, 0u);

  p3a_utils::RecordToHistogramBucket(kTipsSentHistogramName, kTipsSentBuckets,
                                     tip_count);
}

void RecordRecurringTipConfigured(bool tip_configured) {
  UMA_HISTOGRAM_BOOLEAN(kRecurringTipHistogramName, tip_configured);
}

void RecordNoWalletCreatedForAllMetrics() {
  UMA_HISTOGRAM_EXACT_LINEAR(kTipsSentHistogramName, INT_MAX - 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kAutoContributionsStateHistogramName, INT_MAX - 1,
                             2);
}

void RecordRewardsPageViews(PrefService* prefs, bool new_view) {
  if (!prefs->GetBoolean(prefs::kEnabled)) {
    return;
  }
  MonthlyStorage storage(prefs, prefs::kRewardsPageViewCount);
  if (new_view) {
    storage.AddDelta(1);
  }
  auto sum = storage.GetMonthlySum();
  if (sum == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kPageViewCountHistogramName,
                                     kRewardsPageViewCountBuckets, sum);
}

void RecordAdTypesEnabled(PrefService* prefs) {
  if (!prefs->GetBoolean(prefs::kEnabled)) {
    UMA_HISTOGRAM_EXACT_LINEAR(kAdTypesEnabledHistogramName, INT_MAX - 1, 8);
    return;
  }
  int ntp_enabled =
      prefs->GetBoolean(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage);
  int notification_enabled =
      prefs->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds);
  int search_result_enabled =
      prefs->GetBoolean(brave_ads::prefs::kOptedInToSearchResultAds);
  int answer =
      (search_result_enabled << 2) | (notification_enabled << 1) | ntp_enabled;
  UMA_HISTOGRAM_EXACT_LINEAR(kAdTypesEnabledHistogramName, answer, 8);
}

void RecordSearchResultAdsOptinChange(PrefService* prefs) {
  if (prefs->GetBoolean(brave_ads::prefs::kOptedInToSearchResultAds)) {
    UMA_HISTOGRAM_BOOLEAN(kSearchResultAdsOptinHistogramName, true);
  }
}

void RecordAdsHistoryView() {
  UMA_HISTOGRAM_BOOLEAN(kAdsHistoryViewHistogramName, true);
}

ConversionMonitor::ConversionMonitor(PrefService* prefs)
    : prefs_(prefs), panel_trigger_count_(prefs, prefs::kP3APanelTriggerCount) {
  ReportPeriodicMetrics();
}

ConversionMonitor::~ConversionMonitor() = default;

void ConversionMonitor::RecordPanelTrigger(PanelTrigger trigger) {
#if !BUILDFLAG(IS_ANDROID)
  if (prefs_->GetBoolean(prefs::kEnabled)) {
    panel_trigger_count_.AddDelta(1u);
    ReportPanelTriggerCount();
  } else {
    if (trigger == PanelTrigger::kToolbarButton) {
      UMA_HISTOGRAM_EXACT_LINEAR(kToolbarButtonTriggerHistogramName, 1, 2);
    }
    last_trigger_ = trigger;
    last_trigger_time_ = base::Time::Now();
  }
#else
  if (prefs_->GetBoolean(prefs::kEnabled)) {
    panel_trigger_count_.AddDelta(1u);
    ReportPanelTriggerCount();
  } else {
    mobile_trigger_timer_.Start(
        FROM_HERE, kMaxEnabledCauseTriggerTime,
        base::BindOnce(&ConversionMonitor::OnMobileTriggerTimer,
                       base::Unretained(this)));
  }
#endif
}

void ConversionMonitor::RecordRewardsEnable() {
#if !BUILDFLAG(IS_ANDROID)
  // Suspend the other two metrics to prevent overlapping
  // data from being sent once the "rewards enabled source" metric is recorded.
  UMA_HISTOGRAM_EXACT_LINEAR(kToolbarButtonTriggerHistogramName, INT_MAX - 1,
                             2);

  if (!last_trigger_.has_value() ||
      base::Time::Now() - last_trigger_time_ > kMaxEnabledCauseTriggerTime) {
    return;
  }

  UMA_HISTOGRAM_ENUMERATION(kEnabledSourceHistogramName, *last_trigger_);

  last_trigger_.reset();
  last_trigger_time_ = base::Time();
#else
  mobile_trigger_timer_.Stop();
  OnMobileTriggerTimer();
#endif
}

#if BUILDFLAG(IS_ANDROID)
void ConversionMonitor::OnMobileTriggerTimer() {
  UMA_HISTOGRAM_BOOLEAN(kMobileConversionHistogramName,
                        prefs_->GetBoolean(prefs::kEnabled));
}
#endif

void ConversionMonitor::ReportPeriodicMetrics() {
  ReportPanelTriggerCount();
  daily_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval,
                     base::BindOnce(&ConversionMonitor::ReportPeriodicMetrics,
                                    base::Unretained(this)));
}

void ConversionMonitor::ReportPanelTriggerCount() {
  uint64_t total = panel_trigger_count_.GetWeeklySum();
  if (total == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kPanelCountHistogramName,
                                     kPanelCountBuckets, total);
}

}  // namespace brave_rewards::p3a
