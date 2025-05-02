/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_tab_focus_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

constexpr int kSessionCountBuckets[] = {1, 5, 10, 20};
constexpr int kTabCountBuckets[] = {5, 10, 25, 50};

enum class EnabledStatus {
  kDisabled = 0,
  kEnabledFree = 1,
  kEnabledPremium = 2,
  kMaxValue = kEnabledPremium
};

}  // namespace

AIChatTabFocusMetrics::AIChatTabFocusMetrics(PrefService* local_state,
                                             PrefService* profile_prefs,
                                             Delegate* delegate)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      delegate_(delegate),
      total_tab_count_storage_(local_state, prefs::kTabFocusP3ATotalTabCount),
      max_tab_count_storage_(local_state, prefs::kTabFocusP3AMaxTabCount),
      session_count_storage_(local_state, prefs::kTabFocusP3ASessionCount) {
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      prefs::kBraveAIChatTabOrganizationEnabled,
      base::BindRepeating(&AIChatTabFocusMetrics::RecordEnabled,
                          base::Unretained(this)));
}

AIChatTabFocusMetrics::~AIChatTabFocusMetrics() = default;

void AIChatTabFocusMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kTabFocusP3ATotalTabCount);
  registry->RegisterListPref(prefs::kTabFocusP3AMaxTabCount);
  registry->RegisterListPref(prefs::kTabFocusP3ASessionCount);
  registry->RegisterTimePref(prefs::kTabFocusP3ALastUsageTime, {});
}

void AIChatTabFocusMetrics::RecordUsage(size_t tab_count) {
  total_tab_count_storage_.AddDelta(tab_count);
  max_tab_count_storage_.ReplaceTodaysValueIfGreater(tab_count);
  session_count_storage_.AddDelta(1);
  p3a_utils::RecordFeatureUsage(local_state_, nullptr,
                                prefs::kTabFocusP3ALastUsageTime);
  ReportCountMetrics();
  ReportLastUsageTime();
}

void AIChatTabFocusMetrics::RecordEnabled() {
  auto status = EnabledStatus::kDisabled;
  if (profile_prefs_->GetBoolean(prefs::kBraveAIChatTabOrganizationEnabled)) {
    status = delegate_->IsPremium() ? EnabledStatus::kEnabledPremium
                                    : EnabledStatus::kEnabledFree;
  }
  UMA_HISTOGRAM_ENUMERATION(kTabFocusEnabledHistogramName, status);
}

void AIChatTabFocusMetrics::ReportCountMetrics() {
  uint64_t total_sessions = session_count_storage_.GetWeeklySum();
  if (total_sessions == 0) {
    return;
  }

  p3a_utils::RecordToHistogramBucket(kTabFocusSessionCountHistogramName,
                                     kSessionCountBuckets, total_sessions);

  auto total_tabs = total_tab_count_storage_.GetWeeklySum();
  if (total_tabs == 0) {
    return;
  }

  auto avg_tabs =
      static_cast<size_t>(static_cast<double>(total_tabs) / total_sessions);

  p3a_utils::RecordToHistogramBucket(kTabFocusAvgTabCountHistogramName,
                                     kTabCountBuckets, avg_tabs);

  auto max_tabs = max_tab_count_storage_.GetHighestValueInPeriod();
  p3a_utils::RecordToHistogramBucket(kTabFocusMaxTabCountHistogramName,
                                     kTabCountBuckets, max_tabs);
}

void AIChatTabFocusMetrics::ReportLastUsageTime() {
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kTabFocusP3ALastUsageTime,
      kTabFocusLastUsageTimeHistogramName);
}

void AIChatTabFocusMetrics::ReportAllMetrics() {
  RecordEnabled();
  ReportCountMetrics();
  ReportLastUsageTime();
}

}  // namespace ai_chat
