/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_

#include "base/time/time.h"
#include "brave/components/misc_metrics/page_percentage_metrics.h"
#include "build/build_config.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;
class TemplateURLService;

namespace misc_metrics {

inline constexpr char kSearchDailyQueriesBraveDefaultHistogramName[] =
    "Brave.Search.DailyQueries.BraveDefault";
inline constexpr char kSearchDailyQueriesGoogleDefaultHistogramName[] =
    "Brave.Search.DailyQueries.GoogleDefault";
inline constexpr char kSearchDailyQueriesDDGDefaultHistogramName[] =
    "Brave.Search.DailyQueries.DDGDefault";
inline constexpr char kSearchDailyQueriesYahooDefaultHistogramName[] =
    "Brave.Search.DailyQueries.YahooDefault";
inline constexpr char kSearchDailyQueriesOtherDefaultHistogramName[] =
    "Brave.Search.DailyQueries.OtherDefault";
inline constexpr char kSearchOmniboxTypedPercentHistogramName[] =
    "Brave.Search.OmniboxTypedPercent";
inline constexpr char kSearchOmniboxSuggestionPercentHistogramName[] =
    "Brave.Search.OmniboxSuggestionPercent";
inline constexpr char kSearchNTPSearchPercentHistogramName[] =
    "Brave.Search.NTPSearchPercent";
#if BUILDFLAG(IS_ANDROID)
inline constexpr char kSearchWidgetSearchPercentHistogramName[] =
    "Brave.Search.WidgetSearchPercent";
#endif  // BUILDFLAG(IS_ANDROID)

class BraveSearchMetrics : public PagePercentageMetrics {
 public:
  BraveSearchMetrics(PrefService* local_state,
                     TemplateURLService* template_url_service);
  ~BraveSearchMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void MaybeRecordBraveQuery(const GURL& previous_url, const GURL& current_url);
  void MaybeRecordOmniboxQuery(const GURL& destination_url, bool is_suggestion);
  void MaybeRecordNTPSearch(int64_t engine_prepopulate_id);
#if BUILDFLAG(IS_ANDROID)
  void MaybeRecordWidgetSearch(const GURL& url);
#endif  // BUILDFLAG(IS_ANDROID)

  void ClearQueryCounts();

  void ReportAllMetrics();

 private:
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
