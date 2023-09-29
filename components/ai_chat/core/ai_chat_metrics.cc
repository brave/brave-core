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
}  // namespace

const char kChatCountHistogramName[] = "Brave.AIChat.ChatCount";
const char kAvgPromptCountHistogramName[] = "Brave.AIChat.AvgPromptCount";
const char kEnabledHistogramName[] = "Brave.AIChat.Enabled";
const char kUsageDailyHistogramName[] = "Brave.AIChat.UsageDaily";

AIChatMetrics::AIChatMetrics(PrefService* local_state)
    : chat_count_storage_(local_state,
                          prefs::kBraveChatP3AChatCountWeeklyStorage),
      prompt_count_storage_(local_state,
                            prefs::kBraveChatP3APromptCountWeeklyStorage) {
  ReportCounts();
}

AIChatMetrics::~AIChatMetrics() = default;

void AIChatMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveChatP3AChatCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3APromptCountWeeklyStorage);
}

void AIChatMetrics::RecordEnabled() {
  UMA_HISTOGRAM_BOOLEAN(kEnabledHistogramName, true);
}

void AIChatMetrics::RecordNewChat() {
  chat_count_storage_.AddDelta(1);
}

void AIChatMetrics::RecordNewPrompt() {
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageDailyHistogramName, 1, 2);
  prompt_count_storage_.AddDelta(1);
  report_debounce_timer_.Start(
      FROM_HERE, kReportDebounceDelay,
      base::BindOnce(&AIChatMetrics::ReportCounts, AsWeakPtr()));
}

void AIChatMetrics::ReportCounts() {
  uint64_t chat_count = chat_count_storage_.GetWeeklySum();
  periodic_report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&AIChatMetrics::ReportCounts, AsWeakPtr()));

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

}  // namespace ai_chat
