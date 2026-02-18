/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
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

class BraveSearchMetrics {
 public:
  BraveSearchMetrics(PrefService* local_state,
                     TemplateURLService* template_url_service);
  ~BraveSearchMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordBraveQuery();

  void MaybeRecordBraveQuery(const GURL& previous_url, const GURL& current_url);
  void MaybeRecordOmniboxQuery(const GURL& destination_url, bool is_suggestion);
  void MaybeRecordNTPSearch(int64_t engine_prepopulate_id);

  void ClearQueryCounts();

  void ReportAllMetrics();

 private:
  void IncrementDictCount(std::string_view key);

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
