/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/brave_search_metrics.h"

#include "base/containers/fixed_flat_map.h"
#include "base/metrics/histogram_functions.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/daily_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"

namespace misc_metrics {

namespace {

constexpr int kDailyQueriesBuckets[] = {0, 3, 7};

constexpr auto kDailyQueriesHistogramMap =
    base::MakeFixedFlatMap<SearchEngineType, std::string_view>({
        {SEARCH_ENGINE_BRAVE, kSearchDailyQueriesBraveDefaultHistogramName},
        {SEARCH_ENGINE_GOOGLE, kSearchDailyQueriesGoogleDefaultHistogramName},
        {SEARCH_ENGINE_OTHER, kSearchDailyQueriesOtherDefaultHistogramName},
    });

}  // namespace

BraveSearchMetrics::BraveSearchMetrics(PrefService* local_state,
                                       TemplateURLService* template_url_service)
    : local_state_(local_state),
      template_url_service_(template_url_service),
      brave_search_daily_queries_storage_(std::make_unique<DailyStorage>(
          local_state,
          kMiscMetricsBraveSearchDailyQueriesStorage)) {
  template_url_service_observation_.Observe(template_url_service_);
  UpdateSearchEngineType();
}

BraveSearchMetrics::~BraveSearchMetrics() = default;

void BraveSearchMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsBraveSearchDailyQueriesStorage);
}

void BraveSearchMetrics::RecordBraveQuery() {
  brave_search_daily_queries_storage_->RecordValueNow(1);
  ReportDailyQueries();
}

void BraveSearchMetrics::ReportDailyQueries() {
  uint64_t sum = brave_search_daily_queries_storage_->GetLast24HourSum();
  if (sum == 0 || !default_search_engine_type_.has_value()) {
    return;
  }
  for (const auto& [type, name] : kDailyQueriesHistogramMap) {
    if (type == default_search_engine_type_.value()) {
      p3a_utils::RecordToHistogramBucket(name.data(), kDailyQueriesBuckets,
                                         sum);
    } else {
      base::UmaHistogramExactLinear(name.data(), INT_MAX - 1, 4);
    }
  }
}

void BraveSearchMetrics::UpdateSearchEngineType() {
  const TemplateURL* provider =
      template_url_service_->GetDefaultSearchProvider();
  if (!provider) {
    default_search_engine_type_ = std::nullopt;
    return;
  }

  SearchEngineType type =
      provider->GetEngineType(template_url_service_->search_terms_data());
  if (type == SEARCH_ENGINE_BRAVE || type == SEARCH_ENGINE_GOOGLE) {
    default_search_engine_type_ = type;
  } else {
    default_search_engine_type_ = SEARCH_ENGINE_OTHER;
  }
}

void BraveSearchMetrics::OnTemplateURLServiceChanged() {
  UpdateSearchEngineType();
  ReportDailyQueries();
}

}  // namespace misc_metrics
