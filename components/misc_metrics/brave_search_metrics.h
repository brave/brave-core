/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"

class PrefRegistrySimple;
class PrefService;
class TemplateURLService;

namespace misc_metrics {

inline constexpr char kSearchDailyQueriesBraveDefaultHistogramName[] =
    "Brave.Search.DailyQueries.BraveDefault";
inline constexpr char kSearchDailyQueriesGoogleDefaultHistogramName[] =
    "Brave.Search.DailyQueries.GoogleDefault";
inline constexpr char kSearchDailyQueriesOtherDefaultHistogramName[] =
    "Brave.Search.DailyQueries.OtherDefault";

class BraveSearchMetrics {
 public:
  BraveSearchMetrics(PrefService* local_state,
                     TemplateURLService* template_url_service);
  ~BraveSearchMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordBraveQuery();

 private:
  void ReportDailyQueries();

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;

  base::WallClockTimer report_check_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
