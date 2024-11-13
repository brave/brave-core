/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_

#include <memory>
#include <optional>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "build/build_config.h"

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
inline constexpr char kContextMenuLastUsageTimeHistogramName[] =
    "Brave.AIChat.ContextMenu.LastUsageTime";
inline constexpr char kMostUsedContextMenuActionHistogramName[] =
    "Brave.AIChat.ContextMenu.MostUsedAction";
inline constexpr char kContextMenuFreeUsageCountHistogramName[] =
    "Brave.AIChat.ContextMenu.FreeUsages";
inline constexpr char kContextMenuPremiumUsageCountHistogramName[] =
    "Brave.AIChat.ContextMenu.PremiumUsages";
inline constexpr char kEnabledSidebarEnabledAHistogramName[] =
    "Brave.AIChat.Enabled.SidebarEnabledA";
inline constexpr char kEnabledSidebarEnabledBHistogramName[] =
    "Brave.AIChat.Enabled.SidebarEnabledB";
inline constexpr char kUsageDailySidebarEnabledAHistogramName[] =
    "Brave.AIChat.UsageDaily.SidebarEnabledA";
inline constexpr char kUsageDailySidebarEnabledBHistogramName[] =
    "Brave.AIChat.UsageDaily.SidebarEnabledB";
inline constexpr char kUsageWeeklySidebarEnabledAHistogramName[] =
    "Brave.AIChat.UsageWeekly.SidebarEnabledA";
inline constexpr char kUsageWeeklySidebarEnabledBHistogramName[] =
    "Brave.AIChat.UsageWeekly.SidebarEnabledB";
inline constexpr char kChatCountNebulaHistogramName[] =
    "Brave.AIChat.ChatCount.Nebula";
inline constexpr char kMostUsedEntryPointHistogramName[] =
    "Brave.AIChat.MostUsedEntryPoint";

enum class EntryPoint {
  kOmniboxItem = 0,
  kSidebar = 1,
  kContextMenu = 2,
  kToolbarButton = 3,
  kMenuItem = 4,
  kOmniboxCommand = 5,
  kMaxValue = kOmniboxCommand
};

enum class ContextMenuAction {
  kSummarize = 0,
  kExplain = 1,
  kParaphrase = 2,
  kCreateTagline = 3,
  kCreateSocialMedia = 4,
  kImprove = 5,
  kChangeTone = 6,
  kChangeLength = 7,
  kMaxValue = kChangeLength
};

class AIChatMetrics {
 public:
  using RetrievePremiumStatusCallback =
      base::OnceCallback<void(mojom::Service::GetPremiumStatusCallback)>;

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

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  void RecordOmniboxOpen();
  void RecordOmniboxSearchQuery();

  void RecordContextMenuUsage(ContextMenuAction action);
  void HandleOpenViaEntryPoint(EntryPoint entry_point);
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

  void OnPremiumStatusUpdated(bool is_new_user,
                              mojom::PremiumStatus premium_status,
                              mojom::PremiumInfoPtr);

 private:
  void ReportAllMetrics();
  void ReportFeatureUsageMetrics();
  void ReportChatCounts();
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  void ReportOmniboxCounts();
  void ReportContextMenuMetrics();
  void ReportEntryPointUsageMetric();
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

  bool is_enabled_ = false;
  bool is_premium_ = false;
  bool premium_check_in_progress_ = false;
  std::optional<EntryPoint> acquisition_source_ = std::nullopt;

  WeeklyStorage chat_count_storage_;
  WeeklyStorage prompt_count_storage_;
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  base::flat_map<ContextMenuAction, std::unique_ptr<WeeklyStorage>>
      context_menu_usage_storages_;

  TimePeriodStorage omnibox_open_storage_;
  TimePeriodStorage omnibox_autocomplete_storage_;

  base::flat_map<EntryPoint, std::unique_ptr<WeeklyStorage>>
      entry_point_storages_;
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

  base::OneShotTimer report_debounce_timer_;

  base::WallClockTimer periodic_report_timer_;

  raw_ptr<PrefService> local_state_;

  base::WeakPtrFactory<AIChatMetrics> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_METRICS_H_
