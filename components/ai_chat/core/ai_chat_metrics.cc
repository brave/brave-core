/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/ai_chat_metrics.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
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
}  // namespace

const char kChatCountHistogramName[] = "Brave.AIChat.ChatCount";
const char kAvgPromptCountHistogramName[] = "Brave.AIChat.AvgPromptCount";
const char kEnabledHistogramName[] = "Brave.AIChat.Enabled";
const char kUsageDailyHistogramName[] = "Brave.AIChat.UsageDaily";
const char kOmniboxWeekCompareHistogramName[] =
    "Brave.AIChat.OmniboxWeekCompare";
const char kOmniboxOpensHistogramName[] = "Brave.AIChat.OmniboxOpens";
const char kAcquisitionSourceHistogramName[] = "Brave.AIChat.AcquisitionSource";

AIChatMetrics::AIChatMetrics(PrefService* local_state)
    : chat_count_storage_(local_state,
                          prefs::kBraveChatP3AChatCountWeeklyStorage),
      prompt_count_storage_(local_state,
                            prefs::kBraveChatP3APromptCountWeeklyStorage),
      omnibox_open_storage_(local_state,
                            prefs::kBraveChatP3AOmniboxOpenWeeklyStorage,
                            14),
      omnibox_autocomplete_storage_(
          local_state,
          prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage,
          14) {
  ReportAllMetrics();
}

AIChatMetrics::~AIChatMetrics() = default;

void AIChatMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveChatP3AChatCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3APromptCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3AOmniboxOpenWeeklyStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage);
}

void AIChatMetrics::RecordEnabled(bool is_new_user) {
  is_enabled_ = true;
  if (is_new_user) {
    UMA_HISTOGRAM_BOOLEAN(kEnabledHistogramName, true);
    if (acquisition_source_.has_value()) {
      UMA_HISTOGRAM_ENUMERATION(kAcquisitionSourceHistogramName,
                                *acquisition_source_);
    }
    ReportAllMetrics();
  }
}

void AIChatMetrics::RecordNewChat() {
  chat_count_storage_.AddDelta(1);
}

void AIChatMetrics::RecordNewPrompt() {
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageDailyHistogramName, 1, 2);
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
