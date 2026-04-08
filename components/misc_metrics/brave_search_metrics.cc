/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/no_destructor.h"
#include "brave/components/misc_metrics/page_percentage_metrics.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace misc_metrics {

namespace {

constexpr int kDailyQueriesBuckets[] = {0, 3, 7};
constexpr char kQueriesCountKey[] = "queries";
constexpr char kPrimaryQueriesCountKey[] = "primary";
constexpr char kOmniboxTypedCountKey[] = "omnibox_typed";
constexpr char kOmniboxSuggestionCountKey[] = "omnibox_suggestion";
constexpr char kNTPSearchCountKey[] = "ntp_search";
#if BUILDFLAG(IS_ANDROID)
constexpr char kQuickSearchCountKey[] = "quick_search";
constexpr char kQuickSearchKeyword[] = ":br";
constexpr char kWidgetSearchCountKey[] = "widget_search";
#endif  // BUILDFLAG(IS_ANDROID)

bool IsBraveSearchURL(const GURL& url) {
  static const base::NoDestructor<GURL> kBraveSearchURL(
      TemplateURLPrepopulateData::brave_search.search_url);
  return url.host() == kBraveSearchURL->host() &&
         url.path() == kBraveSearchURL->path() && url.has_query();
}

constexpr auto kDailyQueriesHistogramMap =
    base::MakeFixedFlatMap<SearchEngineType, const char*>({
        {SEARCH_ENGINE_BRAVE, kSearchDailyQueriesBraveDefaultHistogramName},
        {SEARCH_ENGINE_GOOGLE, kSearchDailyQueriesGoogleDefaultHistogramName},
        {SEARCH_ENGINE_DUCKDUCKGO, kSearchDailyQueriesDDGDefaultHistogramName},
        {SEARCH_ENGINE_YAHOO, kSearchDailyQueriesYahooDefaultHistogramName},
    });

}  // namespace

BraveSearchMetrics::BraveSearchMetrics(PrefService* local_state,
                                       TemplateURLService* template_url_service)
    : PagePercentageMetrics(local_state,
                            kMiscMetricsBraveSearchQueryCounts,
                            kMiscMetricsBraveSearchReportFrameStartTime),
      template_url_service_(template_url_service) {
  ReportAllMetrics();
}

BraveSearchMetrics::~BraveSearchMetrics() = default;

void BraveSearchMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  CHECK(registry);
  registry->RegisterTimePref(kMiscMetricsBraveSearchReportFrameStartTime, {});
  registry->RegisterDictionaryPref(kMiscMetricsBraveSearchQueryCounts);
}

void BraveSearchMetrics::MaybeRecordBraveQuery(const GURL& previous_url,
                                               const GURL& current_url) {
  if (!IsBraveSearchURL(current_url)) {
    return;
  }

  IncrementDictCount(kQueriesCountKey);

  // Determine if this is a primary query (previous page was NOT a Brave
  // Search results page)
  bool is_primary = !IsBraveSearchURL(previous_url);
  if (is_primary) {
    IncrementDictCount(kPrimaryQueriesCountKey);
  }
}

void BraveSearchMetrics::MaybeRecordOmniboxQuery(const GURL& destination_url,
                                                 bool is_suggestion) {
  if (!IsBraveSearchURL(destination_url)) {
    return;
  }

  auto* key =
      is_suggestion ? kOmniboxSuggestionCountKey : kOmniboxTypedCountKey;
  IncrementDictCount(key);
}

void BraveSearchMetrics::ClearQueryCounts() {
  local_state_->SetDict(kMiscMetricsBraveSearchQueryCounts, {});
}

void BraveSearchMetrics::MaybeRecordNTPSearch(int64_t engine_prepopulate_id) {
  if (engine_prepopulate_id !=
      static_cast<int64_t>(
          TemplateURLPrepopulateData::BravePrepopulatedEngineID::
              PREPOPULATED_ENGINE_ID_BRAVE)) {
    return;
  }
  IncrementDictCount(kNTPSearchCountKey);
}

#if BUILDFLAG(IS_ANDROID)
void BraveSearchMetrics::MaybeRecordQuickSearch(bool is_leo,
                                                std::string_view keyword) {
  if (is_leo || keyword != kQuickSearchKeyword) {
    return;
  }
  IncrementDictCount(kQuickSearchCountKey);
}

void BraveSearchMetrics::MaybeRecordWidgetSearch(const GURL& url) {
  if (!IsBraveSearchURL(url)) {
    return;
  }
  if (last_intent_url_ && *last_intent_url_ == url.spec()) {
    return;
  }
  last_intent_url_ = url.spec();
  IncrementDictCount(kWidgetSearchCountKey);
}
#endif  // BUILDFLAG(IS_ANDROID)

void BraveSearchMetrics::ReportAllMetrics() {
  if (!HasReportIntervalElapsed()) {
    return;
  }

  const TemplateURL* provider =
      template_url_service_->GetDefaultSearchProvider();
  if (!provider) {
    return;
  }

  SearchEngineType engine_type =
      provider->GetEngineType(template_url_service_->search_terms_data());

  const base::DictValue& counts =
      local_state_->GetDict(kMiscMetricsBraveSearchQueryCounts);
  int sum = counts.FindInt(kQueriesCountKey).value_or(0);

  auto* histogram_name_ptr =
      base::FindOrNull(kDailyQueriesHistogramMap, engine_type);
  auto* histogram_name = histogram_name_ptr
                             ? *histogram_name_ptr
                             : kSearchDailyQueriesOtherDefaultHistogramName;

  p3a_utils::RecordToHistogramBucket(histogram_name, kDailyQueriesBuckets, sum);

  int primary_queries = counts.FindInt(kPrimaryQueriesCountKey).value_or(0);
  if (primary_queries > 0) {
    RecordPercentageHistogram(counts, primary_queries, kOmniboxTypedCountKey,
                              kSearchOmniboxTypedPercentHistogramName);
    RecordPercentageHistogram(counts, primary_queries,
                              kOmniboxSuggestionCountKey,
                              kSearchOmniboxSuggestionPercentHistogramName);
    RecordPercentageHistogram(counts, primary_queries, kNTPSearchCountKey,
                              kSearchNTPSearchPercentHistogramName);
#if BUILDFLAG(IS_ANDROID)
    RecordPercentageHistogram(counts, primary_queries, kQuickSearchCountKey,
                              kSearchQuickSearchPercentHistogramName);
    RecordPercentageHistogram(counts, primary_queries, kWidgetSearchCountKey,
                              kSearchWidgetSearchPercentHistogramName);
#endif  // BUILDFLAG(IS_ANDROID)
  }

  ResetCounts();
}

}  // namespace misc_metrics
