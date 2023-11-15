/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_

#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefRegistrySimple;
class PrefService;

namespace ai_chat {

inline constexpr char kChatCountHistogramName[] = "Brave.AIChat.ChatCount";
inline constexpr char kAvgPromptCountHistogramName[] =
    "Brave.AIChat.AvgPromptCount";
inline constexpr char kEnabledHistogramName[] = "Brave.AIChat.Enabled";
inline constexpr char kUsageDailyHistogramName[] = "Brave.AIChat.UsageDaily";
inline constexpr char kOmniboxWeekCompareHistogramName[] =
    "Brave.AIChat.OmniboxWeekCompare";
inline constexpr char kOmniboxOpensHistogramName[] =
    "Brave.AIChat.OmniboxOpens";
inline constexpr char kAcquisitionSourceHistogramName[] =
    "Brave.AIChat.AcquisitionSource";

enum class AcquisitionSource {
  kOmnibox = 0,
  kSidebar = 1,
  kMaxValue = kSidebar
};

class AIChatMetrics {
 public:
  explicit AIChatMetrics(PrefService* local_state);
  ~AIChatMetrics();

  AIChatMetrics(const AIChatMetrics&) = delete;
  AIChatMetrics& operator=(const AIChatMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordEnabled(bool is_new_user);
  void RecordNewChat();
  void RecordNewPrompt();

  void RecordOmniboxOpen();
  void RecordOmniboxSearchQuery();

  void HandleOpenViaSidebar();

 private:
  void ReportAllMetrics();
  void ReportChatCounts();
  void ReportOmniboxCounts();

  bool is_enabled_ = false;
  absl::optional<AcquisitionSource> acquisition_source_ = absl::nullopt;

  WeeklyStorage chat_count_storage_;
  WeeklyStorage prompt_count_storage_;

  TimePeriodStorage omnibox_open_storage_;
  TimePeriodStorage omnibox_autocomplete_storage_;

  base::OneShotTimer report_debounce_timer_;

  base::WallClockTimer periodic_report_timer_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
