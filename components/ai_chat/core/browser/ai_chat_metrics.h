/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace ai_chat {

inline constexpr char kChatCountHistogramName[] = "Brave.AIChat.ChatCount";
inline constexpr char kAvgPromptCountHistogramName[] =
    "Brave.AIChat.AvgPromptCount";
inline constexpr char kEnabledHistogramName[] = "Brave.AIChat.Enabled.2";
inline constexpr char kUsageDailyHistogramName[] = "Brave.AIChat.UsageDaily.2";
inline constexpr char kUsageMonthlyHistogramName[] =
    "Brave.AIChat.UsageMonthly";
inline constexpr char kUsageWeeklyHistogramName[] = "Brave.AIChat.UsageWeekly";
inline constexpr char kOmniboxWeekCompareHistogramName[] =
    "Brave.AIChat.OmniboxWeekCompare";
inline constexpr char kOmniboxOpensHistogramName[] =
    "Brave.AIChat.OmniboxOpens";
inline constexpr char kAcquisitionSourceHistogramName[] =
    "Brave.AIChat.AcquisitionSource";
inline constexpr char kNewUserReturningHistogramName[] =
    "Brave.AIChat.NewUserReturning";
inline constexpr char kLastUsageTimeHistogramName[] =
    "Brave.AIChat.LastUsageTime";

enum class AcquisitionSource {
  kOmnibox = 0,
  kSidebar = 1,
  kMaxValue = kSidebar
};

class AIChatMetrics {
 public:
  using RetrievePremiumStatusCallback =
      base::OnceCallback<void(mojom::PageHandler::GetPremiumStatusCallback)>;

  explicit AIChatMetrics(PrefService* local_state);
  ~AIChatMetrics();

  AIChatMetrics(const AIChatMetrics&) = delete;
  AIChatMetrics& operator=(const AIChatMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordEnabled(
      bool is_enabled,
      bool is_new_user,
      RetrievePremiumStatusCallback retrieve_premium_status_callback);
  void RecordReset();

  void RecordNewChat();
  void RecordNewPrompt();

  void RecordOmniboxOpen();
  void RecordOmniboxSearchQuery();

  void HandleOpenViaSidebar();

  void OnPremiumStatusUpdated(bool is_new_user,
                              mojom::PremiumStatus premium_status,
                              mojom::PremiumInfoPtr);

 private:
  void ReportAllMetrics();
  void ReportFeatureUsageMetrics();
  void ReportChatCounts();
  void ReportOmniboxCounts();

  bool is_enabled_ = false;
  bool is_premium_ = false;
  bool premium_check_in_progress_ = false;
  std::optional<AcquisitionSource> acquisition_source_ = std::nullopt;

  WeeklyStorage chat_count_storage_;
  WeeklyStorage prompt_count_storage_;

  TimePeriodStorage omnibox_open_storage_;
  TimePeriodStorage omnibox_autocomplete_storage_;

  base::OneShotTimer report_debounce_timer_;

  base::WallClockTimer periodic_report_timer_;

  raw_ptr<PrefService> local_state_;

  base::WeakPtrFactory<AIChatMetrics> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
