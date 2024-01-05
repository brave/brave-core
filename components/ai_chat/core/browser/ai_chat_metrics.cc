/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {
constexpr base::TimeDelta kReportInterval = base::Hours(24);
constexpr base::TimeDelta kReportDebounceDelay = base::Seconds(3);
const int kChatCountBuckets[] = {1, 5, 10, 20, 50};
const int kAvgPromptCountBuckets[] = {2, 5, 10, 20};
// Value -1 is added to buckets to add padding for the "less than 1% option"
const int kOmniboxOpenBuckets[] = {-1, 0, 3, 5, 10, 25};

constexpr base::TimeDelta kPremiumCheckInterval = base::Days(1);

}  // namespace

AIChatMetrics::AIChatMetrics(PrefService* local_state)
    : is_premium_(
          local_state->GetBoolean(prefs::kBraveChatP3ALastPremiumStatus)),
      chat_count_storage_(local_state,
                          prefs::kBraveChatP3AChatCountWeeklyStorage),
      prompt_count_storage_(local_state,
                            prefs::kBraveChatP3APromptCountWeeklyStorage),
      omnibox_open_storage_(local_state,
                            prefs::kBraveChatP3AOmniboxOpenWeeklyStorage,
                            14),
      omnibox_autocomplete_storage_(
          local_state,
          prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage,
          14),
      local_state_(local_state) {}

AIChatMetrics::~AIChatMetrics() = default;

void AIChatMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveChatP3AChatCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3APromptCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3AOmniboxOpenWeeklyStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage);
  registry->RegisterTimePref(prefs::kBraveChatP3ALastPremiumCheck,
                             base::Time());
  registry->RegisterBooleanPref(prefs::kBraveChatP3ALastPremiumStatus, false);
  registry->RegisterTimePref(prefs::kBraveChatP3AFirstUsageTime, base::Time());
  registry->RegisterTimePref(prefs::kBraveChatP3ALastUsageTime, base::Time());
  registry->RegisterBooleanPref(prefs::kBraveChatP3AUsedSecondDay, false);
}

void AIChatMetrics::RecordEnabled(
    bool is_enabled,
    bool is_new_user,
    RetrievePremiumStatusCallback retrieve_premium_status_callback) {
  if (is_enabled && !is_new_user &&
      local_state_->GetTime(prefs::kBraveChatP3AFirstUsageTime).is_null()) {
    // If the user already had AI chat enabled, and we did not record the first
    // & last usage time, set the first & last usage time to a date 90 days ago
    // so we don't skew feature usage metrics.
    base::Time three_months_ago = base::Time::Now() - base::Days(90);
    local_state_->SetTime(prefs::kBraveChatP3AFirstUsageTime, three_months_ago);
    local_state_->SetTime(prefs::kBraveChatP3ALastUsageTime, three_months_ago);
  }

  if (!is_enabled) {
    ReportFeatureUsageMetrics();
    return;
  }

  if (retrieve_premium_status_callback) {
    base::Time last_premium_check =
        local_state_->GetTime(prefs::kBraveChatP3ALastPremiumCheck);
    if (last_premium_check.is_null() ||
        (base::Time::Now() - last_premium_check) >= kPremiumCheckInterval) {
      if (!premium_check_in_progress_) {
        premium_check_in_progress_ = true;
        std::move(retrieve_premium_status_callback)
            .Run(base::BindOnce(&AIChatMetrics::OnPremiumStatusUpdated,
                                weak_ptr_factory_.GetWeakPtr(), is_new_user));
      }
      return;
    }
  }

  is_enabled_ = true;

  UMA_HISTOGRAM_EXACT_LINEAR(kEnabledHistogramName, is_premium_ ? 2 : 1, 3);
  if (is_new_user && acquisition_source_.has_value()) {
    UMA_HISTOGRAM_ENUMERATION(kAcquisitionSourceHistogramName,
                              *acquisition_source_);
  }

  ReportAllMetrics();
}

void AIChatMetrics::RecordReset() {
  UMA_HISTOGRAM_EXACT_LINEAR(kEnabledHistogramName,
                             std::numeric_limits<int>::max() - 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kAcquisitionSourceHistogramName,
                             std::numeric_limits<int>::max() - 1, 2);
}

void AIChatMetrics::OnPremiumStatusUpdated(bool is_new_user,
                                           mojom::PremiumStatus premium_status,
                                           mojom::PremiumInfoPtr) {
  is_premium_ = premium_status == mojom::PremiumStatus::Active ||
                premium_status == mojom::PremiumStatus::ActiveDisconnected;
  local_state_->SetBoolean(prefs::kBraveChatP3ALastPremiumStatus, is_premium_);
  local_state_->SetTime(prefs::kBraveChatP3ALastPremiumCheck,
                        base::Time::Now());
  premium_check_in_progress_ = false;
  RecordEnabled(true, is_new_user, {});
}

void AIChatMetrics::RecordNewChat() {
  chat_count_storage_.AddDelta(1);
}

void AIChatMetrics::RecordNewPrompt() {
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageDailyHistogramName, is_premium_ ? 2 : 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageWeeklyHistogramName, is_premium_ ? 2 : 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageMonthlyHistogramName, is_premium_ ? 2 : 1,
                             3);
  p3a_utils::RecordFeatureUsage(local_state_,
                                prefs::kBraveChatP3AFirstUsageTime,
                                prefs::kBraveChatP3ALastUsageTime);
  ReportFeatureUsageMetrics();
  prompt_count_storage_.AddDelta(1);
  report_debounce_timer_.Start(
      FROM_HERE, kReportDebounceDelay,
      base::BindOnce(&AIChatMetrics::ReportChatCounts, base::Unretained(this)));
}

void AIChatMetrics::RecordOmniboxOpen() {
  acquisition_source_ = AcquisitionSource::kOmnibox;
  omnibox_open_storage_.AddDelta(1);
  omnibox_autocomplete_storage_.AddDelta(1);
  ReportOmniboxCounts();
}

void AIChatMetrics::RecordOmniboxSearchQuery() {
  omnibox_autocomplete_storage_.AddDelta(1);
  ReportOmniboxCounts();
}

void AIChatMetrics::HandleOpenViaSidebar() {
  acquisition_source_ = AcquisitionSource::kSidebar;
}

void AIChatMetrics::ReportAllMetrics() {
  periodic_report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&AIChatMetrics::ReportAllMetrics, base::Unretained(this)));
  ReportChatCounts();
  ReportOmniboxCounts();
  ReportFeatureUsageMetrics();
}

void AIChatMetrics::ReportFeatureUsageMetrics() {
  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, prefs::kBraveChatP3AFirstUsageTime,
      prefs::kBraveChatP3ALastUsageTime, prefs::kBraveChatP3AUsedSecondDay,
      kNewUserReturningHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveChatP3ALastUsageTime,
      kLastUsageTimeHistogramName, true);
}

void AIChatMetrics::ReportChatCounts() {
  uint64_t chat_count = chat_count_storage_.GetWeeklySum();

  if (chat_count == 0) {
    // Do not report if AI chat was not used in the past week.
    return;
  }

  uint64_t prompt_count = prompt_count_storage_.GetWeeklySum();
  int average_prompts_per_chat = static_cast<int>(
      std::ceil(static_cast<double>(prompt_count) /
                std::max(chat_count, static_cast<uint64_t>(1))));

  p3a_utils::RecordToHistogramBucket(kChatCountHistogramName, kChatCountBuckets,
                                     chat_count);
  p3a_utils::RecordToHistogramBucket(kAvgPromptCountHistogramName,
                                     kAvgPromptCountBuckets,
                                     average_prompts_per_chat);
}

void AIChatMetrics::ReportOmniboxCounts() {
  if (!is_enabled_) {
    return;
  }

  base::Time today_midnight = base::Time::Now();
  base::Time one_week_ago = today_midnight - base::Days(7);
  base::Time two_weeks_ago = today_midnight - base::Days(14);
  uint64_t autocomplete_count_this_week =
      omnibox_autocomplete_storage_.GetPeriodSumInTimeRange(one_week_ago,
                                                            today_midnight);
  uint64_t autocomplete_count_last_week =
      omnibox_autocomplete_storage_.GetPeriodSumInTimeRange(two_weeks_ago,
                                                            one_week_ago);
  uint64_t open_count_this_week = omnibox_open_storage_.GetPeriodSumInTimeRange(
      one_week_ago, today_midnight);
  uint64_t open_count_last_week = omnibox_open_storage_.GetPeriodSumInTimeRange(
      two_weeks_ago, one_week_ago);

  double open_ratio_this_week =
      static_cast<double>(open_count_this_week) /
      std::max(autocomplete_count_this_week, static_cast<uint64_t>(1));
  if (open_ratio_this_week == 0.0) {
    UMA_HISTOGRAM_EXACT_LINEAR(kOmniboxOpensHistogramName, 0, 7);
  } else {
    p3a_utils::RecordToHistogramBucket(
        kOmniboxOpensHistogramName, kOmniboxOpenBuckets,
        static_cast<int>(open_ratio_this_week * 100));
  }

  double open_ratio_last_week =
      static_cast<double>(open_count_last_week) /
      std::max(autocomplete_count_last_week, static_cast<uint64_t>(1));
  if (open_ratio_last_week > 0.0) {
    UMA_HISTOGRAM_BOOLEAN(kOmniboxWeekCompareHistogramName,
                          open_ratio_this_week > open_ratio_last_week);
  }
}

}  // namespace ai_chat
