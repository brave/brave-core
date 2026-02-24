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
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace misc_metrics {

namespace {

constexpr int kDailyQueriesBuckets[] = {0, 3, 7};
constexpr char kQueriesCountKey[] = "queries";
constexpr base::TimeDelta kReportInterval = base::Hours(24);
constexpr base::TimeDelta kReportCheckInterval = base::Minutes(30);

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
  registry->RegisterTimePref(kMiscMetricsBraveSearchReportFrameStartTime, {});
  registry->RegisterDictionaryPref(kMiscMetricsBraveSearchQueryCounts);
}

void BraveSearchMetrics::RecordBraveQuery() {
  ScopedDictPrefUpdate update(local_state_, kMiscMetricsBraveSearchQueryCounts);
  int current = update->FindInt(kQueriesCountKey).value_or(0);
  update->Set(kQueriesCountKey, current + 1);
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

  const base::Value::Dict& counts =
      local_state_->GetDict(kMiscMetricsBraveSearchQueryCounts);
  int sum = counts.FindInt(kQueriesCountKey).value_or(0);

  auto* histogram_name_ptr =
      base::FindOrNull(kDailyQueriesHistogramMap, engine_type);
  auto* histogram_name = histogram_name_ptr
                             ? *histogram_name_ptr
                             : kSearchDailyQueriesOtherDefaultHistogramName;

  p3a_utils::RecordToHistogramBucket(histogram_name, kDailyQueriesBuckets, sum);

  local_state_->SetTime(kMiscMetricsBraveSearchReportFrameStartTime, now);
  local_state_->SetDict(kMiscMetricsBraveSearchQueryCounts, {});
}

}  // namespace misc_metrics
