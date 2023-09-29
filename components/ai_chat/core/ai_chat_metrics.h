/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_METRICS_H_

#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace ai_chat {

extern const char kChatCountHistogramName[];
extern const char kAvgPromptCountHistogramName[];
extern const char kEnabledHistogramName[];
extern const char kUsageDailyHistogramName[];

class AIChatMetrics : public base::SupportsWeakPtr<AIChatMetrics> {
 public:
  explicit AIChatMetrics(PrefService* local_state);
  ~AIChatMetrics();

  AIChatMetrics(const AIChatMetrics&) = delete;
  AIChatMetrics& operator=(const AIChatMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordEnabled();
  void RecordNewChat();
  void RecordNewPrompt();

 private:
  void ReportCounts();

  WeeklyStorage chat_count_storage_;
  WeeklyStorage prompt_count_storage_;

  base::OneShotTimer report_debounce_timer_;
  base::WallClockTimer periodic_report_timer_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_AI_CHAT_METRICS_H_
