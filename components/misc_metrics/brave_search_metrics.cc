/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_functions.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace misc_metrics {

namespace {

constexpr char kBraveSearchHost[] = "search.brave.com";
constexpr char kBraveSearchPath[] = "/search";
constexpr int kDailyQueriesBuckets[] = {0, 3, 7};
constexpr int kEntryPercentageBuckets[] = {0, 5, 20, 80, 95};
constexpr char kQueriesCountKey[] = "queries";
constexpr char kPrimaryQueriesCountKey[] = "primary";
constexpr char kOmniboxTypedCountKey[] = "omnibox_typed";
constexpr char kOmniboxSuggestionCountKey[] = "omnibox_suggestion";
constexpr char kNTPSearchCountKey[] = "ntp_search";
constexpr base::TimeDelta kReportInterval = base::Hours(24);
constexpr base::TimeDelta kReportCheckInterval = base::Minutes(30);

bool IsBraveSearchURL(const GURL& url) {
  return url.host() == kBraveSearchHost && url.path() == kBraveSearchPath &&
         url.has_query();
}

constexpr auto kDailyQueriesHistogramMap =
    base::MakeFixedFlatMap<SearchEngineType, const char*>({
        {SEARCH_ENGINE_BRAVE, kSearchDailyQueriesBraveDefaultHistogramName},
        {SEARCH_ENGINE_GOOGLE, kSearchDailyQueriesGoogleDefaultHistogramName},
        {SEARCH_ENGINE_DUCKDUCKGO, kSearchDailyQueriesDDGDefaultHistogramName},
        {SEARCH_ENGINE_YAHOO, kSearchDailyQueriesYahooDefaultHistogramName},
    });

void RecordSearchPercentage(const base::Value::Dict& counts,
                            int primary_queries,
                            std::string_view count_key,
                            const char* histogram_name) {
  int count = counts.FindInt(count_key).value_or(0);
  if (count == 0) {
    return;
  }
  // Calculate percentage, ensuring we report at least 1% to avoid bucket 0
  int percentage = std::max(1, (count * 100) / primary_queries);
  p3a_utils::RecordToHistogramBucket(histogram_name, kEntryPercentageBuckets,
                                     percentage);
}

}  // namespace

BraveSearchMetrics::BraveSearchMetrics(PrefService* local_state,
                                       TemplateURLService* template_url_service)
    : local_state_(local_state), template_url_service_(template_url_service) {
  base::Time frame_start =
      local_state_->GetTime(kMiscMetricsBraveSearchReportFrameStartTime);
  if (frame_start.is_null()) {
    local_state_->SetTime(kMiscMetricsBraveSearchReportFrameStartTime,
                          base::Time::Now());
  }

  ReportDailyQueries();
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
      static_cast<int64_t>(TemplateURLPrepopulateData::BravePrepopulatedEngineID::
          PREPOPULATED_ENGINE_ID_BRAVE)) {
    return;
  }
  IncrementDictCount(kNTPSearchCountKey);
}

void BraveSearchMetrics::ReportDailyQueries() {
  report_check_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportCheckInterval,
      base::BindOnce(&BraveSearchMetrics::ReportDailyQueries,
                     base::Unretained(this)));

  base::Time frame_start =
      local_state_->GetTime(kMiscMetricsBraveSearchReportFrameStartTime);
  base::Time now = base::Time::Now();
  if (now - frame_start < kReportInterval) {
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

  // Report omnibox entry percentages
  int primary_queries = counts.FindInt(kPrimaryQueriesCountKey).value_or(0);
  if (primary_queries > 0) {
    RecordSearchPercentage(counts, primary_queries, kOmniboxTypedCountKey,
                           kSearchOmniboxTypedPercentHistogramName);
    RecordSearchPercentage(counts, primary_queries, kOmniboxSuggestionCountKey,
                           kSearchOmniboxSuggestionPercentHistogramName);
    RecordSearchPercentage(counts, primary_queries, kNTPSearchCountKey,
                           kSearchNTPSearchPercentHistogramName);
  }

  local_state_->SetTime(kMiscMetricsBraveSearchReportFrameStartTime, now);
  local_state_->SetDict(kMiscMetricsBraveSearchQueryCounts, {});
}

void BraveSearchMetrics::IncrementDictCount(std::string_view key) {
  ScopedDictPrefUpdate update(local_state_, kMiscMetricsBraveSearchQueryCounts);
  int current = update->FindInt(key).value_or(0);
  update->Set(key, current + 1);
}

}  // namespace misc_metrics
