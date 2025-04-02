/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_TAB_FOCUS_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_TAB_FOCUS_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"

class PrefService;

namespace ai_chat {

inline constexpr char kTabFocusAvgTabCountHistogramName[] =
    "Brave.AIChat.TabFocus.AvgTabCount";
inline constexpr char kTabFocusMaxTabCountHistogramName[] =
    "Brave.AIChat.TabFocus.MaxTabCount";
inline constexpr char kTabFocusEnabledHistogramName[] =
    "Brave.AIChat.TabFocus.Enabled";
inline constexpr char kTabFocusSessionCountHistogramName[] =
    "Brave.AIChat.TabFocus.SessionCount";
inline constexpr char kTabFocusLastUsageTimeHistogramName[] =
    "Brave.AIChat.TabFocus.LastUsageTime";

class AIChatTabFocusMetrics {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual bool IsPremium() const = 0;
  };

  AIChatTabFocusMetrics(PrefService* local_state,
                        PrefService* profile_prefs,
                        Delegate* delegate);
  ~AIChatTabFocusMetrics();

  AIChatTabFocusMetrics(const AIChatTabFocusMetrics&) = delete;
  AIChatTabFocusMetrics& operator=(const AIChatTabFocusMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordUsage(size_t tab_count);

  void ReportAllMetrics();

 private:
  void ReportCountMetrics();
  void RecordEnabled();
  void ReportLastUsageTime();

  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<Delegate> delegate_;

  PrefChangeRegistrar pref_change_registrar_;

  WeeklyStorage total_tab_count_storage_;
  WeeklyStorage max_tab_count_storage_;
  WeeklyStorage session_count_storage_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_TAB_FOCUS_METRICS_H_
