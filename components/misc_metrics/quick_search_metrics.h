/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_QUICK_SEARCH_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_QUICK_SEARCH_METRICS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

class PrefRegistrySimple;
class PrefService;
class TemplateURLService;
class WeeklyStorage;

namespace misc_metrics {

inline constexpr char kQuickSearchMostUsedActionHistogramName[] =
    "Brave.Search.QuickMostUsedAction";

enum class Action {
  kLeo = 1,
  kDefaultEngine = 2,
  kGoogle = 3,
  kYouTube = 4,
  kBing = 5,
  kEcosia = 6,
  kDuckDuckGo = 7,
  kQwant = 8,
  kStartpage = 9,
  kBrave = 10,
  kOther = 11,
  kMaxValue = kOther,
};

// Reports metrics for the Android quick search feature.
class QuickSearchMetrics {
 public:
  QuickSearchMetrics(PrefService* local_state,
                     TemplateURLService* template_url_service);
  ~QuickSearchMetrics();

  QuickSearchMetrics(const QuickSearchMetrics&) = delete;
  QuickSearchMetrics& operator=(const QuickSearchMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordQuickSearch(bool is_leo, const std::string& keyword);

 private:
  Action ResolveAction(bool is_leo, const std::string& keyword);
  void MaybeInitStorage();
  void ReportMostUsedAction();
  void UpdateMetrics();
  void SetUpTimer();

  raw_ptr<PrefService> local_state_;
  raw_ptr<TemplateURLService> template_url_service_;
  absl::flat_hash_map<Action, std::unique_ptr<WeeklyStorage>> action_storages_;
  base::WallClockTimer report_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_QUICK_SEARCH_METRICS_H_
