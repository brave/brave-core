/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/quick_search_metrics.h"

#include <optional>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportUpdateInterval = base::Days(1);

constexpr auto kActionKeys = base::MakeFixedFlatMap<Action, const char*>({
    {Action::kLeo, "leo"},
    {Action::kDefaultEngine, "default_engine"},
    {Action::kGoogle, "google"},
    {Action::kYouTube, "youtube"},
    {Action::kBing, "bing"},
    {Action::kEcosia, "ecosia"},
    {Action::kDuckDuckGo, "duckduckgo"},
    {Action::kQwant, "qwant"},
    {Action::kStartpage, "startpage"},
    {Action::kOther, "other"},
});

constexpr auto kKeywordToAction =
    base::MakeFixedFlatMap<std::string_view, Action>({
        {":g", Action::kGoogle},
        {":yt", Action::kYouTube},
        {":b", Action::kBing},
        {":e", Action::kEcosia},
        {":d", Action::kDuckDuckGo},
        {":dl", Action::kDuckDuckGo},
        {":q", Action::kQwant},
        {":sp", Action::kStartpage},
    });

}  // namespace

QuickSearchMetrics::QuickSearchMetrics(PrefService* local_state,
                                       TemplateURLService* template_url_service)
    : local_state_(local_state), template_url_service_(template_url_service) {
  UpdateMetrics();
}

QuickSearchMetrics::~QuickSearchMetrics() = default;

void QuickSearchMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  CHECK(registry);
  registry->RegisterDictionaryPref(kMiscMetricsQuickSearchActionStorage);
  registry->RegisterTimePref(kMiscMetricsQuickSearchLastClickTime, {});
}

void QuickSearchMetrics::RecordQuickSearch(bool is_leo,
                                           const std::string& keyword) {
  MaybeInitStorage();
  local_state_->SetTime(kMiscMetricsQuickSearchLastClickTime,
                        base::Time::Now());
  Action action = ResolveAction(is_leo, keyword);
  auto* storage = base::FindPtrOrNull(action_storages_, action);
  CHECK(storage);
  storage->AddDelta(1);
  ReportMostUsedAction();
}

Action QuickSearchMetrics::ResolveAction(bool is_leo,
                                         const std::string& keyword) {
  if (is_leo) {
    return Action::kLeo;
  }

  if (!keyword.empty() && template_url_service_) {
    const TemplateURL* default_provider =
        template_url_service_->GetDefaultSearchProvider();
    if (default_provider &&
        default_provider->keyword() == base::UTF8ToUTF16(keyword)) {
      return Action::kDefaultEngine;
    }
  }

  if (auto* action = base::FindOrNull(kKeywordToAction, keyword)) {
    return *action;
  }

  return Action::kOther;
}

void QuickSearchMetrics::MaybeInitStorage() {
  for (int i = 1; i <= static_cast<int>(Action::kMaxValue); ++i) {
    Action action = static_cast<Action>(i);
    if (action_storages_.contains(action)) {
      continue;
    }
    auto* key = base::FindOrNull(kActionKeys, action);
    CHECK(key);
    action_storages_[action] = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsQuickSearchActionStorage, *key);
  }
}

void QuickSearchMetrics::ReportMostUsedAction() {
  base::Time last_click =
      local_state_->GetTime(kMiscMetricsQuickSearchLastClickTime);
  if (last_click.is_null() ||
      (base::Time::Now() - last_click) > base::Days(7)) {
    return;
  }

  MaybeInitStorage();

  uint64_t max_count = 0;
  std::optional<Action> most_used;

  // Iterate through enum values in order to ensure deterministic behavior
  for (int i = 1; i <= static_cast<int>(Action::kMaxValue); ++i) {
    Action action = static_cast<Action>(i);
    auto* storage = base::FindPtrOrNull(action_storages_, action);
    CHECK(storage);
    uint64_t sum = storage->GetWeeklySum();
    if (sum > max_count) {
      max_count = sum;
      most_used = action;
    }
  }

  if (most_used) {
    UMA_HISTOGRAM_ENUMERATION(kQuickSearchMostUsedActionHistogramName,
                              *most_used);
  }
}

void QuickSearchMetrics::UpdateMetrics() {
  ReportMostUsedAction();
  SetUpTimer();
}

void QuickSearchMetrics::SetUpTimer() {
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportUpdateInterval,
                      base::BindOnce(&QuickSearchMetrics::UpdateMetrics,
                                     base::Unretained(this)));
}

}  // namespace misc_metrics
